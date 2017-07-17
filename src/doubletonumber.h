#ifndef DOUBLETONUMER_H
#define DOUBLETONUMBER_H

#include "bignum.h"

#define SCALE_NAN 0x80000000
#define SCALE_INF 0x7FFFFFFF
#define NUMBER_MAXDIGITS 50

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
        numerator.setUInt64(realMantissa);
        BigNum::shiftLeft(&numerator, realExponent);

        // Explanation:
        // value = (realMantissa * 2^realExponent) / (1)
        denominator.setUInt64(1);
    }
    else
    {
        // Explanation:
        // value = (realMantissa * 2^realExponent) / (1)
        //       = (realMantissa / 2^(-realExponent)
        numerator.setUInt64(realMantissa);
        BigNum::shiftLeft(1, -realExponent, denominator);
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

    uint32_t hiBlock = scaledDenominator.getBlockValue(scaledDenominator.length() - 1);
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

        BigNum::shiftLeft(&scaledDenominator, shift);
        BigNum::shiftLeft(&scaledNumerator, shift);
    }

    int digitsNum = 0;
    int currentDigit = 0;
    while (BigNum::compare(scaledNumerator, 0) > 0 && digitsNum < count)
    {
        currentDigit = BigNum::heuristicDivide(&scaledNumerator, scaledDenominator);
        if (BigNum::compare(scaledNumerator, 0) == 0 || digitsNum + 1 == count)
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

#endif // BIGNUM_H