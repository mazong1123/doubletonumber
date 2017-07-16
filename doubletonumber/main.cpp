#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cassert>

using namespace std;

#define SCALE_NAN 0x80000000
#define SCALE_INF 0x7FFFFFFF
#define NUMBER_MAXDIGITS 50

#define SLL(x, y, z, k) {\
   uint32_t x_sll = (x);\
   (z) = (x_sll << (y)) | (k);\
   (k) = x_sll >> (32 - (y));\
}

class BigNum
{
public:
    BigNum()
        :m_len(0)
    {
    }

    BigNum(uint32_t value)
    {
        setUInt32(value);
    }

    BigNum(uint64_t value)
    {
        setUInt64(value);
    }

    ~BigNum()
    {
    }

    BigNum & operator=(const BigNum &rhs)
    {
        uint8_t length = rhs.m_len;
        uint32_t* pCurrent = m_blocks;
        const uint32_t* pRhsCurrent = rhs.m_blocks;
        const uint32_t* pRhsEnd = pRhsCurrent + length;

        while (pRhsCurrent != pRhsEnd)
        {
            *pCurrent = *pRhsCurrent;

            ++pCurrent;
            ++pRhsCurrent;
        }

        m_len = length;

        return *this;
    }

    uint8_t length() const
    {
        return m_len;
    }

    static int compare(const BigNum& lhs, uint32_t value)
    {
        if (lhs.m_len == 0)
        {
            return -1;
        }

        uint32_t lhsValue = lhs.m_blocks[0];

        if (lhsValue > value)
        {
            return 1;
        }

        if (lhsValue < value)
        {
            return -1;
        }

        return 0;
    }

    static int compare(const BigNum& lhs, const BigNum& rhs)
    {
        int lenDiff = lhs.m_len - rhs.m_len;
        if (lenDiff != 0)
        {
            return lenDiff;
        }

        for (uint8_t i = 0; i < lhs.m_len; ++i)
        {
            if (lhs.m_blocks[i] == rhs.m_blocks[i])
            {
                continue;
            }

            if (lhs.m_blocks[i] > rhs.m_blocks[i])
            {
                return 1;
            }
            else if (lhs.m_blocks[i] < rhs.m_blocks[i])
            {
                return -1;
            }
        }

        return 0;
    }

    static void pow10(int exp, BigNum& result)
    {
        BigNum temp1;
        BigNum temp2;

        BigNum* pCurrentTemp = &temp1;
        BigNum* pNextTemp = &temp2;

        uint32_t smallExp = exp & 0x7;
        pCurrentTemp->setUInt32(m_power10UInt32Table[smallExp]);

        exp >>= 3;
        uint32_t idx = 0;

        while (exp != 0)
        {
            // if the current bit is set, multiply it with the corresponding power of 10
            if (exp & 1)
            {
                // multiply into the next temporary
                multiply(*pCurrentTemp, m_power10BigNumTable[idx], *pNextTemp);

                // swap to the next temporary
                swap(pNextTemp, pCurrentTemp);
            }

            // advance to the next bit
            ++idx;
            exp >>= 1;
        }

        result = *pCurrentTemp;
    }

    static uint32_t divdeRoundDown(const BigNum& lhs, const BigNum& rhs)
    {
        return 0;
    }

    static void subtract(const BigNum& lhs, const BigNum& rhs, BigNum& result)
    {
        assert(lhs.m_len >= rhs.m_len);
        // TODO: assert lhs >= rhs.

        const uint32_t* pLhsCurrent = lhs.m_blocks;
        const uint32_t* pLhsEnd = pLhsCurrent + lhs.m_len;

        const uint32_t* pRhsCurrent = rhs.m_blocks;
        const uint32_t* pRhsEnd = rhs.m_blocks + rhs.m_len;

        uint32_t* pResultCurrent = result.m_blocks;
        uint8_t len = 0;
        bool isBorrow = false;

        while (pRhsCurrent != pRhsEnd)
        {
            if (isBorrow)
            {
                // TODO: may have a bug if *pLhsCurrent - 1 < *pRhsCurrent.
                // We should add the borrowed value.

                *pResultCurrent = *pLhsCurrent - *pRhsCurrent - 1;
                isBorrow = *pRhsCurrent >= *pLhsCurrent;
            }
            else
            {
                // TODO: may have a bug if *pLhsCurrent < *pRhsCurrent.
                // We should add the borrowed value.

                *pResultCurrent = *pLhsCurrent - *pRhsCurrent;
                isBorrow = *pRhsCurrent > *pLhsCurrent;
            }

            ++pResultCurrent;
            ++pRhsCurrent;
            ++pLhsCurrent;
        }

        uint8_t lenDiff = lhs.m_len - rhs.m_len;

        uint8_t start = lenDiff;
        for (uint8_t start = lenDiff; isBorrow && start > 0; --start)
        {
            uint32_t sub = *pLhsCurrent++;
            *pResultCurrent++ = sub - 1;
            isBorrow = (sub == 0);
        }

        while (start > 0)
        {
            *pResultCurrent++ = *pLhsCurrent++;
            --start;
        }

        len = lhs.m_len;

        while (len > 0 && *pResultCurrent == 0)
        {
            --len;
            --pResultCurrent;
        }

        result.m_len = len;
    }

    static void multiply(const BigNum& lhs, uint32_t value, BigNum& result)
    {
        if (lhs.m_len == 0)
        {
            return;
        }

        // Zero out result internal blocks.
        memset(result.m_blocks, 0, sizeof(uint32_t) * BIGSIZE);

        const uint32_t* pCurrent = lhs.m_blocks;
        const uint32_t* pEnd = pCurrent + lhs.m_len;
        uint32_t* pResultCurrent = result.m_blocks;

        uint64_t carry = 0;
        while (pCurrent != pEnd)
        {
            uint64_t product = (uint64_t)(*pCurrent) * (uint64_t)value + carry;
            carry = product >> 32;
            *pResultCurrent = (uint32_t)(product & 0xFFFFFFFF);

            ++pResultCurrent;
            ++pCurrent;
        }

        if (lhs.m_len < BIGSIZE && result.m_blocks[lhs.m_len] != 0)
        {
            result.m_len = lhs.m_len + 1;
        }
        else
        {
            result.m_len = lhs.m_len;
        }
    }

    static void multiply(const BigNum& lhs, const BigNum& rhs, BigNum& result)
    {
        const BigNum* pLarge = NULL;
        const BigNum* pSmall = NULL;
        if (lhs.m_len < rhs.m_len)
        {
            pSmall = &lhs;
            pLarge = &rhs;
        }
        else
        {
            pSmall = &rhs;
            pLarge = &lhs;
        }

        uint8_t maxResultLength = pSmall->m_len + pLarge->m_len;

        // Zero out result internal blocks.
        memset(result.m_blocks, 0, sizeof(uint32_t) * BIGSIZE);

        const uint32_t* pLargeBegin = pLarge->m_blocks;
        const uint32_t* pLargeEnd = pLarge->m_blocks + pLarge->m_len;

        uint32_t* pResultStart = result.m_blocks;
        const uint32_t* pSmallCurrent = pSmall->m_blocks;
        const uint32_t* pSmallEnd = pSmallCurrent + pSmall->m_len;

        while (pSmallCurrent != pSmallEnd)
        {
            // Multiply each block of large BigNum.
            if (*pSmallCurrent != 0)
            {
                const uint32_t* pLargeCurrent = pLargeBegin;
                uint32_t* pResultCurrent = pResultStart;
                uint64_t carry = 0;

                do
                {
                    uint64_t product = (uint64_t)(*pResultCurrent) + (uint64_t)(*pSmallCurrent) * (uint64_t)(*pLargeCurrent) + carry;
                    carry = product >> 32;
                    *pResultCurrent = (uint32_t)(product & 0xFFFFFFFF);

                    ++pResultCurrent;
                    ++pLargeCurrent;
                } while (pLargeCurrent != pLargeEnd);

                *pResultCurrent = (uint32_t)(carry & 0xFFFFFFFF);
            }

            ++pSmallCurrent;
            ++pResultStart;
        }

        if (maxResultLength > 0 && result.m_blocks[maxResultLength - 1] == 0)
        {
            result.m_len = maxResultLength - 1;
        }
        else
        {
            result.m_len = maxResultLength;
        }
    }

    void setUInt32(uint32_t value)
    {
        m_len = 1;
        m_blocks[0] = value;
    }

    void setUInt64(uint64_t value)
    {
        m_len = 0;
        m_blocks[0] = (uint32_t)(value & 0xFFFFFFFF);
        m_len++;

        uint32_t highBits = (uint32_t)(value >> 32);
        if (highBits != 0)
        {
            m_blocks[1] = highBits;
            m_len++;
        }
    }

    void extendBlock(uint32_t newBlock)
    {
        m_blocks[m_len] = newBlock;
        ++m_len;
    }

private:
    static const uint8_t BIGSIZE = 24;
    static const uint8_t UINT32POWER10NUM = 8;
    static const uint8_t BIGPOWER10NUM = 6;
    static uint32_t m_power10UInt32Table[UINT32POWER10NUM];
    static BigNum m_power10BigNumTable[BIGPOWER10NUM];

    static class StaticInitializer
    {
    public:
        StaticInitializer()
        {
            // 10^8
            m_power10BigNumTable[0].m_len = (uint8_t)1;
            m_power10BigNumTable[0].m_blocks[0] = (uint32_t)100000000;

            // 10^16
            m_power10BigNumTable[1].m_len = (uint8_t)2;
            m_power10BigNumTable[1].m_blocks[0] = (uint32_t)0x6fc10000;
            m_power10BigNumTable[1].m_blocks[1] = (uint32_t)0x002386f2;

            // 10^32
            m_power10BigNumTable[2].m_len = (uint8_t)4;
            m_power10BigNumTable[2].m_blocks[0] = (uint32_t)0x00000000;
            m_power10BigNumTable[2].m_blocks[1] = (uint32_t)0x85acef81;
            m_power10BigNumTable[2].m_blocks[2] = (uint32_t)0x2d6d415b;
            m_power10BigNumTable[2].m_blocks[3] = (uint32_t)0x000004ee;

            // 10^64
            m_power10BigNumTable[3].m_len = (uint8_t)7;
            m_power10BigNumTable[3].m_blocks[0] = (uint32_t)0x00000000;
            m_power10BigNumTable[3].m_blocks[1] = (uint32_t)0x00000000;
            m_power10BigNumTable[3].m_blocks[2] = (uint32_t)0xbf6a1f01;
            m_power10BigNumTable[3].m_blocks[3] = (uint32_t)0x6e38ed64;
            m_power10BigNumTable[3].m_blocks[4] = (uint32_t)0xdaa797ed;
            m_power10BigNumTable[3].m_blocks[5] = (uint32_t)0xe93ff9f4;
            m_power10BigNumTable[3].m_blocks[6] = (uint32_t)0x00184f03;

            // 10^128
            m_power10BigNumTable[4].m_len = (uint8_t)14;
            m_power10BigNumTable[4].m_blocks[0] = (uint32_t)0x00000000;
            m_power10BigNumTable[4].m_blocks[1] = (uint32_t)0x00000000;
            m_power10BigNumTable[4].m_blocks[2] = (uint32_t)0x00000000;
            m_power10BigNumTable[4].m_blocks[3] = (uint32_t)0x00000000;
            m_power10BigNumTable[4].m_blocks[4] = (uint32_t)0x2e953e01;
            m_power10BigNumTable[4].m_blocks[5] = (uint32_t)0x03df9909;
            m_power10BigNumTable[4].m_blocks[6] = (uint32_t)0x0f1538fd;
            m_power10BigNumTable[4].m_blocks[7] = (uint32_t)0x2374e42f;
            m_power10BigNumTable[4].m_blocks[8] = (uint32_t)0xd3cff5ec;
            m_power10BigNumTable[4].m_blocks[9] = (uint32_t)0xc404dc08;
            m_power10BigNumTable[4].m_blocks[10] = (uint32_t)0xbccdb0da;
            m_power10BigNumTable[4].m_blocks[11] = (uint32_t)0xa6337f19;
            m_power10BigNumTable[4].m_blocks[12] = (uint32_t)0xe91f2603;
            m_power10BigNumTable[4].m_blocks[13] = (uint32_t)0x0000024e;

            // 10^256
            m_power10BigNumTable[5].m_len = (uint8_t)27;
            m_power10BigNumTable[5].m_blocks[0] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[1] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[2] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[3] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[4] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[5] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[6] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[7] = (uint32_t)0x00000000;
            m_power10BigNumTable[5].m_blocks[8] = (uint32_t)0x982e7c01;
            m_power10BigNumTable[5].m_blocks[9] = (uint32_t)0xbed3875b;
            m_power10BigNumTable[5].m_blocks[10] = (uint32_t)0xd8d99f72;
            m_power10BigNumTable[5].m_blocks[11] = (uint32_t)0x12152f87;
            m_power10BigNumTable[5].m_blocks[12] = (uint32_t)0x6bde50c6;
            m_power10BigNumTable[5].m_blocks[13] = (uint32_t)0xcf4a6e70;
            m_power10BigNumTable[5].m_blocks[14] = (uint32_t)0xd595d80f;
            m_power10BigNumTable[5].m_blocks[15] = (uint32_t)0x26b2716e;
            m_power10BigNumTable[5].m_blocks[16] = (uint32_t)0xadc666b0;
            m_power10BigNumTable[5].m_blocks[17] = (uint32_t)0x1d153624;
            m_power10BigNumTable[5].m_blocks[18] = (uint32_t)0x3c42d35a;
            m_power10BigNumTable[5].m_blocks[19] = (uint32_t)0x63ff540e;
            m_power10BigNumTable[5].m_blocks[20] = (uint32_t)0xcc5573c0;
            m_power10BigNumTable[5].m_blocks[21] = (uint32_t)0x65f9ef17;
            m_power10BigNumTable[5].m_blocks[22] = (uint32_t)0x55bc28f2;
            m_power10BigNumTable[5].m_blocks[23] = (uint32_t)0x80dcc7f7;
            m_power10BigNumTable[5].m_blocks[24] = (uint32_t)0xf46eeddc;
            m_power10BigNumTable[5].m_blocks[25] = (uint32_t)0x5fdcefce;
            m_power10BigNumTable[5].m_blocks[26] = (uint32_t)0x000553f7;
        }
    } m_initializer;

    uint8_t m_len;
    uint32_t m_blocks[BIGSIZE];
};

uint32_t BigNum::m_power10UInt32Table[UINT32POWER10NUM] =
{
    1,          // 10^0
    10,         // 10^1
    100,        // 10^2
    1000,       // 10^3
    10000,      // 10^4
    100000,     // 10^5
    1000000,    // 10^6
    10000000,   // 10^7
};

BigNum BigNum::m_power10BigNumTable[BIGPOWER10NUM];
BigNum::StaticInitializer BigNum::m_initializer;

struct NUMBER
{
    int precision;
    int scale;
    int sign;
    wchar_t digits[NUMBER_MAXDIGITS + 1];
    wchar_t* allDigits;
    NUMBER() : precision(0), scale(0), sign(0), allDigits(NULL) {}
};

struct FPDOUBLE
{
#if BIGENDIAN
    unsigned int sign : 1;
    unsigned int exp : 11;
    unsigned int mantHi : 20;
    unsigned int mantLo;
#else
    unsigned int mantLo;
    unsigned int mantHi : 20;
    unsigned int exp : 11;
    unsigned int sign : 1;
#endif
};

void uint64ShiftLeft(uint64_t input, int shift, BigNum& output)
{
    /*BigNum rr;
    BigNum b1;
    b1.setUInt64(4294967295);

    BigNum b2;
    b2.setUInt32(100);

    BigNum::multiply(b1, b2, rr);*/

    int shiftBlocks = shift / 32;
    int remaningToShiftBits = shift % 32;

    for (int i = 0; i < shiftBlocks; ++i)
    {
        // If blocks shifted, we should fill the corresponding blocks with zero.
        output.extendBlock(0);
    }

    if (remaningToShiftBits == 0)
    {
        // We shift 32 * n (n >= 1) bits. No remaining bits.
        output.extendBlock((uint32_t)(input & 0xFFFFFFFF));

        uint32_t highBits = (uint32_t)(input >> 32);
        if (highBits != 0)
        {
            output.extendBlock(highBits);
        }
    }
    else
    {
        // Extract the high position bits which would be shifted out of range.
        uint32_t highPositionBits = (uint32_t)input >> (32 + 32 - remaningToShiftBits);

        // Shift the input. The result should be stored to current block.
        uint64_t shiftedInput = input << remaningToShiftBits;
        output.extendBlock(shiftedInput & 0xFFFFFFFF);

        uint32_t highBits = (uint32_t)(input >> 32);
        if (highBits != 0)
        {
            output.extendBlock(highBits);
        }

        if (highPositionBits != 0)
        {
            // If the high position bits is not 0, we should store them to next block.
            output.extendBlock(highPositionBits);
        }
    }
}

/*void bigShiftLeft(BigNum* input, int shiftBits, BigNum* output)
{
    int n, m, i, xl, zl;
    uint64_t *xp, *zp, k;

    n = shiftBits / 64;
    m = shiftBits % 64;
    xl = input->l;
    xp = &(input->d[0]);
    zl = xl + n;
    zp = &(output->d[0]);
    for (i = n; i > 0; i--)
    {
        *zp++ = 0;
    }

    if (m == 0)
    {
        for (i = xl; i >= 0; i--)
        {
            *zp++ = *xp++;
        }
    }
    else
    {
        for (i = xl, k = 0; i >= 0; i--)
        {
            SLL(*xp++, m, *zp++, k);
        }
        if (k != 0)
        {
            *zp = k, zl++;
        }
    }
    output->l = zl;
}*/

double log10F(double v)
{
    return 0;
}

uint32_t logBase2(uint32_t val)
{
    static const uint8_t logTable[256] =
    {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
    };

    uint32_t temp;

    temp = val >> 24;
    if (temp)
    {
        return 24 + logTable[temp];
    }

    temp = val >> 16;
    if (temp)
    {
        return 16 + logTable[temp];
    }

    temp = val >> 8;
    if (temp)
    {
        return 8 + logTable[temp];
    }

    return logTable[val];
}

uint32_t logBase2(uint64_t val)
{
    uint64_t temp;

    temp = val >> 32;
    if (temp)
    {
        return 32 + logBase2((uint32_t)temp);
    }

    return logBase2((uint32_t)val);
}

char * __cdecl
_ecvt2(double value, int count, int * dec, int * sign)
{
    //BigNum test;
    //shortShiftLeft((uint64_t)8796093022207, 40, &test);
    //shortShiftLeft((uint64_t)100, 3, &test);
    uint64_t realMantissa = ((uint64_t)(((FPDOUBLE*)&value)->mantHi) << 32) | ((FPDOUBLE*)&value)->mantLo;
    int realExponent = -1074;
    uint32_t mantissaHighBitIdx = 0;
    if (((FPDOUBLE*)&value)->exp > 0)
    {
        realMantissa += ((uint64_t)1 << 52);
        realExponent = ((FPDOUBLE*)&value)->exp - 1075;
        mantissaHighBitIdx = 52;
    }
    else
    {
        mantissaHighBitIdx = logBase2(realMantissa);
    }

    char* digits = (char *)malloc(count + 1);

    const double log10_2 = 0.30102999566398119521373889472449;
    int firstDigitExponent = (int)(ceil(double((int)mantissaHighBitIdx + realExponent) * log10_2 - 0.69));

    BigNum numerator;
    BigNum denominator;
    if (realExponent > 0)
    {
        // value = (realMantissa * 2^realExponent) / (1)
        uint64ShiftLeft(realMantissa, realExponent, numerator);
        denominator.setUInt64(1);
    }
    else
    {
        // value = (realMantissa * 2^realExponent) / (1)
        //       = (realMantissa / 2^(-realExponent)
        numerator.setUInt64(realMantissa);
        uint64ShiftLeft(1, -realExponent, denominator);
    }

    // TODO: Avoid copies!
    BigNum scaledNumerator;
    BigNum scaledDenominator;

    if (firstDigitExponent > 0)
    {
        BigNum poweredValue;
        BigNum::pow10(firstDigitExponent, poweredValue);
        BigNum::multiply(denominator, poweredValue, scaledDenominator);
        scaledNumerator = numerator;
    }
    else if (firstDigitExponent < 0)
    {
        BigNum poweredValue;
        BigNum::pow10(-firstDigitExponent, poweredValue);
        BigNum::multiply(numerator, poweredValue, scaledNumerator);
        scaledDenominator = denominator;
    }

    if (BigNum::compare(scaledNumerator, scaledDenominator) >= 0)
    {
        // The exponent estimation was incorrect.
        firstDigitExponent += 1;
    }

    int digitsNum = 0;
    while (BigNum::compare(numerator, 0) > 0 && digitsNum < count)
    {
        uint32_t digit = BigNum::divdeRoundDown(scaledNumerator, scaledDenominator);
        digits[digitsNum] = '0' + digit;
        ++digitsNum;

        BigNum tempNumerator;
        BigNum multipliedDenominator;
        BigNum::multiply(scaledDenominator, digit, multipliedDenominator);
        BigNum::subtract(scaledNumerator, multipliedDenominator, tempNumerator);

        BigNum newNumerator;
        BigNum::multiply(tempNumerator, (uint32_t)10, newNumerator);

        numerator = newNumerator;
    }

    *dec = firstDigitExponent;
    *sign = ((FPDOUBLE*)&value)->sign;

    return digits;
}

void DoubleToNumber(double value, int precision, NUMBER* number)
{
    number->precision = precision;
    if (((FPDOUBLE*)&value)->exp == 0x7FF)
    {
        number->scale = (((FPDOUBLE*)&value)->mantLo || ((FPDOUBLE*)&value)->mantHi) ? SCALE_NAN : SCALE_INF;
        number->sign = ((FPDOUBLE*)&value)->sign;
        number->digits[0] = 0;
    }
    else
    {
        char* src = _ecvt2(value, precision, &number->scale, &number->sign);
        wchar_t* dst = number->digits;
        if (*src != '0')
        {
            while (*src) *dst++ = *src++;
        }
        *dst = 0;
    }
}

int main()
{
    BigNum r;
    r.setUInt64(1111111111111111111);

    BigNum r2;
    r2.setUInt32(100);

    BigNum rr;
    BigNum::subtract(r, r2, rr);

    //BigNum d = m_power10BigNumTable[0];
    NUMBER number;
    //DoubleToNumber(7.9228162514264338e+28, 15, &number);
    DoubleToNumber(122.5, 15, &number);

    return 0;
}