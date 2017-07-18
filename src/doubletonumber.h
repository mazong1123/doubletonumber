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

char * __cdecl
_ecvt2(double value, int count, int * dec, int * sign)
{
    // Step 1: 
    // Extract meta data from the input double value.
    //
    // Refer to IEEE double precision floating point format.
    uint64_t realMantissa = 0;
    int realExponent = 0;
    uint32_t mantissaHighBitIdx = 0;
    if (((FPDOUBLE*)&value)->exp > 0)
    {
        realMantissa = ((uint64_t)(((FPDOUBLE*)&value)->mantHi) << 32) | ((FPDOUBLE*)&value)->mantLo + ((uint64_t)1 << 52);
        realExponent = ((FPDOUBLE*)&value)->exp - 1075;
        mantissaHighBitIdx = 52;
    }
    else
    {
        realMantissa = ((uint64_t)(((FPDOUBLE*)&value)->mantHi) << 32) | ((FPDOUBLE*)&value)->mantLo;
        realExponent = -1074;
        mantissaHighBitIdx = BigNum::logBase2(realMantissa);
    }

    char* digits = (char *)malloc(count + 1);

    // Step 2:
    // Calculate the first digit exponent. We should estimate the exponent and then verify it later.
    //
    // This is an improvement of the estimation in the original paper.
    // Inspired by http://www.ryanjuckett.com/programming/printing-floating-point-numbers/
    //
    // 0.30102999566398119521373889472449 = log10V2
    // 0.69 = 1 - log10V2 - epsilon (a small number account for drift of floating point multiplication)
    int firstDigitExponent = (int)(ceil(double((int)mantissaHighBitIdx + realExponent) * 0.30102999566398119521373889472449 - 0.69));

    // Step 3:
    // Store the input double value in BigNum format.
    //
    // To keep the precision, we represent the double value as numertor/denominator.
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

    if (firstDigitExponent > 0)
    {
        BigNum poweredValue;
        BigNum::pow10(firstDigitExponent, poweredValue);
        denominator.multiply(poweredValue);
    }
    else if (firstDigitExponent < 0)
    {
        BigNum poweredValue;
        BigNum::pow10(-firstDigitExponent, poweredValue);
        numerator.multiply(poweredValue);
    }

    if (BigNum::compare(numerator, denominator) >= 0)
    {
        // The exponent estimation was incorrect.
        firstDigitExponent += 1;
    }
    else
    {
        numerator.multiply(10);
    }

    *dec = firstDigitExponent - 1;

    BigNum::prepareHeuristicDivide(&numerator, &denominator);

    // Step 4:
    // Calculate digits.
    //
    // Output digits until reaching the last but one precision or the numerator becomes zero.
    int digitsNum = 0;
    int currentDigit = 0;
    while (true)
    {
        currentDigit = BigNum::heuristicDivide(&numerator, denominator);
        if (numerator.isZero() || digitsNum + 1 == count)
        {
            break;
        }

        digits[digitsNum] = '0' + currentDigit;
        ++digitsNum;

        numerator.multiply(10);
    }

    // Step 5:
    // Set the last digit.
    //
    // We round to the closest digit by comparing value with 0.5:
    //  compare( value, 0.5 )
    //  = compare( numerator / denominator, 0.5 )
    //  = compare( numerator, 0.5 * denominator)
    //  = compare(2 * numerator, denominator)
    numerator.multiply(2);
    int compareResult = BigNum::compare(numerator, denominator);
    bool isRoundDown = compareResult < 0;

    // We are in the middle, round towards the even digit (i.e. IEEE rouding rules)
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

        // Rounding up for 9 is special.
        if (currentDigit == 9)
        {
            // find the first non-nine prior digit
            while (true)
            {
                // If we are at the first digit
                if (pCurDigit == digits)
                {
                    // Output 1 at the next highest exponent
                    *pCurDigit = '1';
                    ++digitsNum;
                    *dec += 1;
                    break;
                }

                --pCurDigit;
                --digitsNum;
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
            // It's simple if the digit is not 9.
            *pCurDigit = '0' + currentDigit + 1;
            ++digitsNum;
        }
    }

    if (digitsNum < count)
    {
        memset(digits + digitsNum, '0', count - digitsNum);
    }

    digits[count] = 0;

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