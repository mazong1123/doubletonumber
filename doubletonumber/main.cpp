#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>

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

    ~BigNum()
    {
    }

    uint8_t length() const
    {
        return m_len;
    }

    static void pow10(const int value, BigNum& result)
    {
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
    uint8_t m_len;
    uint32_t m_blocks[BIGSIZE];
};

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

char * __cdecl
_ecvt2(double value, int count, int * dec, int * sign)
{
    //BigNum test;
    //shortShiftLeft((uint64_t)8796093022207, 40, &test);
    //shortShiftLeft((uint64_t)100, 3, &test);
    uint64_t realMantissa = ((uint64_t)(((FPDOUBLE*)&value)->mantHi) << 32) | ((FPDOUBLE*)&value)->mantLo;
    int realExponent = -1074;
    if (((FPDOUBLE*)&value)->exp > 0)
    {
        realMantissa += ((uint64_t)1 << 52);
        realExponent = ((FPDOUBLE*)&value)->exp - 1075;
    }

    char* digits = (char *)malloc(count + 1);

    int firstDigitExponent = (int)ceill(log10F(value));

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

    if (firstDigitExponent > 0)
    {
        BigNum poweredValue;
        BigNum::pow10(firstDigitExponent, poweredValue);
        BigNum::multiply(denominator, poweredValue, denominator);
    }
    else if (firstDigitExponent < 0)
    {
        BigNum poweredValue;
        BigNum::pow10(firstDigitExponent, poweredValue);
        BigNum::multiply(numerator, poweredValue, numerator);
    }

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
    NUMBER number;
    //DoubleToNumber(7.9228162514264338e+28, 15, &number);
    DoubleToNumber(122.5, 15, &number);

    return 0;
}