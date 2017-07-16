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

    uint32_t getBlock(uint8_t index)
    {
        return m_blocks[index];
    }

    static int compare(const BigNum& lhs, uint32_t value)
    {
        if (lhs.m_len == 0)
        {
            return -1;
        }

        uint32_t lhsValue = lhs.m_blocks[0];

        if (lhsValue > value || lhs.m_len > 1)
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

        for (int i = lhs.m_len - 1; i >= 0; --i)
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

    static void bigIntShiftLeft(BigNum* pResult, uint32_t shift)
    {
        uint32_t shiftBlocks = shift / 32;
        uint32_t shiftBits = shift % 32;

        // process blocks high to low so that we can safely process in place
        const uint32_t* pInBlocks = pResult->m_blocks;
        int inLength = pResult->m_len;

        // check if the shift is block aligned
        if (shiftBits == 0)
        {
            // copy blcoks from high to low
            for (uint32_t * pInCur = pResult->m_blocks + inLength, *pOutCur = pInCur + shiftBlocks;
                pInCur >= pInBlocks;
                --pInCur, --pOutCur)
            {
                *pOutCur = *pInCur;
            }

            // zero the remaining low blocks
            for (uint32_t i = 0; i < shiftBlocks; ++i)
                pResult->m_blocks[i] = 0;

            pResult->m_len += shiftBlocks;
        }
        // else we need to shift partial blocks
        else
        {
            int inBlockIdx = inLength - 1;
            uint32_t outBlockIdx = inLength + shiftBlocks;

            // set the length to hold the shifted blocks
            pResult->m_len = outBlockIdx + 1;

            // output the initial blocks
            const uint32_t lowBitsShift = (32 - shiftBits);
            uint32_t highBits = 0;
            uint32_t block = pResult->m_blocks[inBlockIdx];
            uint32_t lowBits = block >> lowBitsShift;
            while (inBlockIdx > 0)
            {
                pResult->m_blocks[outBlockIdx] = highBits | lowBits;
                highBits = block << shiftBits;

                --inBlockIdx;
                --outBlockIdx;

                block = pResult->m_blocks[inBlockIdx];
                lowBits = block >> lowBitsShift;
            }

            // output the final blocks
            pResult->m_blocks[outBlockIdx] = highBits | lowBits;
            pResult->m_blocks[outBlockIdx - 1] = block << shiftBits;

            // zero the remaining low blocks
            for (uint32_t i = 0; i < shiftBlocks; ++i)
                pResult->m_blocks[i] = 0;

            // check if the terminating block has no set bits
            if (pResult->m_blocks[pResult->m_len - 1] == 0)
                --pResult->m_len;
        }
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

    static uint32_t divdeRoundDown(BigNum* pDividend, const BigNum& divisor)
    {
        uint32_t len = divisor.m_len;
        if (pDividend->m_len < len)
        {
            return 0;
        }

        const uint32_t* pFinalDivisorBlock = divisor.m_blocks + len - 1;
        uint32_t* pFinalDividendBlock = pDividend->m_blocks + len - 1;

        uint32_t quotient = *pFinalDividendBlock / *pFinalDivisorBlock;
        // Divide out the estimated quotient
        if (quotient != 0)
        {
            // dividend = dividend - divisor*quotient
            const uint32_t *pDivisorCur = divisor.m_blocks;
            uint32_t *pDividendCur = pDividend->m_blocks;

            uint64_t borrow = 0;
            uint64_t carry = 0;
            do
            {
                uint64_t product = (uint64_t)*pDivisorCur * (uint64_t)quotient + carry;
                carry = product >> 32;

                uint64_t difference = (uint64_t)*pDividendCur - (product & 0xFFFFFFFF) - borrow;
                borrow = (difference >> 32) & 1;

                *pDividendCur = difference & 0xFFFFFFFF;

                ++pDivisorCur;
                ++pDividendCur;
            } while (pDivisorCur <= pFinalDivisorBlock);

            // remove all leading zero blocks from dividend
            while (len > 0 && pDividend->m_blocks[len - 1] == 0)
            {
                --len;
            }

            pDividend->m_len = len;
        }

        // If the dividend is still larger than the divisor, we overshot our estimate quotient. To correct,
        // we increment the quotient and subtract one more divisor from the dividend.
        if (BigNum::compare(*pDividend, divisor) >= 0)
        {
            ++quotient;

            // dividend = dividend - divisor
            const uint32_t *pDivisorCur = divisor.m_blocks;
            uint32_t *pDividendCur = pDividend->m_blocks;

            uint64_t borrow = 0;
            do
            {
                uint64_t difference = (uint64_t)*pDividendCur - (uint64_t)*pDivisorCur - borrow;
                borrow = (difference >> 32) & 1;

                *pDividendCur = difference & 0xFFFFFFFF;

                ++pDivisorCur;
                ++pDividendCur;
            } while (pDivisorCur <= pFinalDivisorBlock);

            // remove all leading zero blocks from dividend
            while (len > 0 && pDividend->m_blocks[len - 1] == 0)
            {
                --len;
            }

            pDividend->m_len = len;
        }

        return quotient;
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

        while (len > 0 && *--pResultCurrent == 0)
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
    memset(digits, 0, count + 1);

    const double log10_2 = 0.30102999566398119521373889472449;
    int firstDigitExponent = (int)(ceil(double((int)mantissaHighBitIdx + realExponent) * log10_2 - 0.69));

    BigNum numerator;
    BigNum denominator;
    if (realExponent > 0)
    {
        numerator.setUInt64(4 * realMantissa);
        BigNum::bigIntShiftLeft(&numerator, realExponent);
        // value = (realMantissa * 2^realExponent) / (1)
        //uint64ShiftLeft(4 * realMantissa, realExponent, numerator);
        denominator.setUInt64(4);
    }
    else
    {
        // value = (realMantissa * 2^realExponent) / (1)
        //       = (realMantissa / 2^(-realExponent)
        numerator.setUInt64(2 * realMantissa);
        uint64ShiftLeft(1, -realExponent + 1, denominator);
    }

    // TODO: Avoid copies!
    BigNum scaledNumerator = numerator;
    BigNum scaledDenominator = denominator;

    if (firstDigitExponent > 0)
    {
        BigNum poweredValue;
        BigNum::pow10(firstDigitExponent, poweredValue);
        BigNum::multiply(denominator, poweredValue, scaledDenominator);
    }
    else if (firstDigitExponent < 0)
    {
        BigNum poweredValue;
        BigNum::pow10(-firstDigitExponent, poweredValue);
        BigNum::multiply(numerator, poweredValue, scaledNumerator);
    }

    if (BigNum::compare(scaledNumerator, scaledDenominator) >= 0)
    {
        // The exponent estimation was incorrect.
        firstDigitExponent += 1;
    }
    else
    {
        BigNum temp;
        BigNum::multiply(scaledNumerator, 10, temp);
        scaledNumerator = temp;
    }

    *dec = firstDigitExponent - 1;

    uint32_t hiBlock = scaledDenominator.getBlock(scaledDenominator.length() - 1);
    if (hiBlock < 8 || hiBlock > 429496729)
    {
        // Perform a bit shift on all values to get the highest block of the denominator into
        // the range [8,429496729]. We are more likely to make accurate quotient estimations
        // in BigInt_DivideWithRemainder_MaxQuotient9() with higher denominator values so
        // we shift the denominator to place the highest bit at index 27 of the highest block.
        // This is safe because (2^28 - 1) = 268435455 which is less than 429496729. This means
        // that all values with a highest bit at index 27 are within range.         
        uint32_t hiBlockLog2 = logBase2(hiBlock);
        uint32_t shift = (32 + 27 - hiBlockLog2) % 32;

        BigNum::bigIntShiftLeft(&scaledDenominator, shift);
        BigNum::bigIntShiftLeft(&scaledNumerator, shift);
        /*BigInt_ShiftLeft(&scale, shift);
        BigInt_ShiftLeft(&scaledValue, shift);
        BigInt_ShiftLeft(&scaledMarginLow, shift);
        if (pScaledMarginHigh != &scaledMarginLow)
            BigInt_Multiply2(pScaledMarginHigh, scaledMarginLow);*/
    }

    int digitsNum = 0;
    int currentDigit = 0;
    while (BigNum::compare(scaledNumerator, 0) > 0 && digitsNum < count)
    {
        currentDigit = BigNum::divdeRoundDown(&scaledNumerator, scaledDenominator);
        if (digitsNum + 1 == count)
        {
            break;
        }

        if (currentDigit != 0 || digitsNum > 0)
        {
            digits[digitsNum] = '0' + currentDigit;
            ++digitsNum;
        }

        /*BigNum tempNumerator;
        BigNum multipliedDenominator;
        BigNum::multiply(scaledDenominator, currentDigit, multipliedDenominator);
        BigNum::subtract(scaledNumerator, multipliedDenominator, tempNumerator);*/

        BigNum newNumerator;
        BigNum::multiply(scaledNumerator, (uint32_t)10, newNumerator);

        scaledNumerator = newNumerator;
    }

    // Set last digit. We need to decide round down or round up.
    // round to the closest digit by comparing value with 0.5. To do this we need to convert
    // the inequality to large integer values.
    //  compare( value, 0.5 )
    //  = compare( scaledNumerator / scaledDenominator, 0.5 )
    //  = compare( scaledNumerator, 0.5 * scaledDenominator)
    //  = compare(2 * scaledNumberator, scaledDenominator)
    BigNum tempScaledNumerator;
    BigNum::multiply(scaledNumerator, 2, tempScaledNumerator);

    int compareResult = BigNum::compare(tempScaledNumerator, scaledDenominator);
    bool isRoundDown = compareResult < 0;

    // if we are directly in the middle, round towards the even digit (i.e. IEEE rouding rules)
    if (compareResult == 0)
    {
        isRoundDown = (currentDigit & 1) == 0;
    }

    if (isRoundDown)
    {
        digits[digitsNum] = '0' + currentDigit;
        ++digitsNum;
    }
    else
    {
        char* pCurDigit = digits + digitsNum;

        // handle rounding up
        if (currentDigit == 9)
        {
            // find the first non-nine prior digit
            for (;;)
            {
                // if we are at the first digit
                if (pCurDigit == digits)
                {
                    // output 1 at the next highest exponent
                    *pCurDigit = '1';
                    ++digitsNum;
                    *dec += 1;
                    break;
                }

                --pCurDigit;
                if (*pCurDigit != '9')
                {
                    // increment the digit
                    *pCurDigit += 1;
                    ++digitsNum;
                    break;
                }
            }
        }
        else
        {
            // values in the range [0,8] can perform a simple round up
            *pCurDigit = '0' + currentDigit + 1;
            ++digitsNum;
        }
    }

    if (digitsNum < count)
    {
        memset(digits + digitsNum, '0', count - digitsNum);
    }

    digits[count] = '\0';

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

uint64_t getMantissa(double d)
{
    return (*(uint64_t*)&d) & 0x000FFFFFFFFFFFFF;
}

int main()
{
    BigNum r;
    r.setUInt64(100);

    BigNum r2;
    r2.setUInt32(100);

    BigNum rr;
    BigNum::subtract(r, r2, rr);

    //BigNum d = m_power10BigNumTable[0];
    NUMBER number;

    //uint64_t dd = getMantissa(7.9228162514264339123);
    //DoubleToNumber(7.9228162514264338e+28, 17, &number);
    //DoubleToNumber(70.9228162514264339123, 17, &number);
    //DoubleToNumber(70.9228162514264339123, 27, &number);
    DoubleToNumber(-1.79769313486231E+308, 17, &number);

    return 0;
}