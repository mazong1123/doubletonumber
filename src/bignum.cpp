#include "bignum.h"
#include <algorithm>
#include <cassert>

using std::swap;

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

BigNum::BigNum()
    :m_len(0)
{
}

BigNum::BigNum(uint32_t value)
{
    setUInt32(value);
}

BigNum::BigNum(uint64_t value)
{
    setUInt64(value);
}

BigNum::~BigNum()
{
}

BigNum& BigNum::operator=(const BigNum &rhs)
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

uint8_t BigNum::length() const
{
    return m_len;
}

uint32_t BigNum::getBlockValue(uint8_t index)
{
    return m_blocks[index];
}

int BigNum::compare(const BigNum& lhs, uint32_t value)
{
    if (lhs.m_len == 0)
    {
        return value == 0 ? 0 : -1;
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

int BigNum::compare(const BigNum& lhs, const BigNum& rhs)
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

void BigNum::bigIntShiftLeft(BigNum* pResult, uint32_t shift)
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

void BigNum::pow10(int exp, BigNum& result)
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

uint32_t BigNum::divdeRoundDown(BigNum* pDividend, const BigNum& divisor)
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

void BigNum::subtract(const BigNum& lhs, const BigNum& rhs, BigNum& result)
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

void BigNum::multiply(const BigNum& lhs, uint32_t value, BigNum& result)
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

void BigNum::multiply(const BigNum& lhs, const BigNum& rhs, BigNum& result)
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

void BigNum::setUInt32(uint32_t value)
{
    m_len = 1;
    m_blocks[0] = value;
}

void BigNum::setUInt64(uint64_t value)
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

void BigNum::extendBlock(uint32_t newBlock)
{
    m_blocks[m_len] = newBlock;
    ++m_len;
}