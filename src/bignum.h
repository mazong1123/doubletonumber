#ifndef BIGNUM_H
#define BIGNUM_H

#include <cmath>
#include <cstdint>
#include <algorithm>

class BigNum
{
public:
    BigNum();
    BigNum(uint32_t value);
    BigNum(uint64_t value);
    ~BigNum();

    BigNum & operator=(const BigNum &rhs);

    static uint32_t logBase2(uint32_t val);
    static uint32_t logBase2(uint64_t val);

    static int compare(const BigNum& lhs, uint32_t value);
    static int compare(const BigNum& lhs, const BigNum& rhs);

    static void shiftLeft(uint64_t input, int shift, BigNum& output);
    static void shiftLeft(BigNum* pResult, uint32_t shift);
    static void pow10(int exp, BigNum& result);
    static void prepareHeuristicDivide(BigNum* pDividend, BigNum* divisor);
    static uint32_t heuristicDivide(BigNum* pDividend, const BigNum& divisor);
    static void multiply(const BigNum& lhs, uint32_t value, BigNum& result);
    static void multiply(const BigNum& lhs, const BigNum& rhs, BigNum& result);

    void setUInt32(uint32_t value);
    void setUInt64(uint64_t value);
    void extendBlock(uint32_t newBlock);

private:

    static const uint8_t BIGSIZE = 35;
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

#endif // BIGNUM_H