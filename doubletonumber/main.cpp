#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>

using namespace std;

#define SCALE_NAN 0x80000000
#define SCALE_INF 0x7FFFFFFF
#define NUMBER_MAXDIGITS 50
#define BIGSIZE 24

#define SLL(x, y, z, k) {\
   uint64_t x_sll = (x);\
   (z) = (x_sll << (y)) | (k);\
   (k) = x_sll >> (64 - (y));\
}

struct BigNum
{
    int len;
    uint64_t blocks[BIGSIZE];
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

void shortShiftLeft(uint64_t input, int shift, BigNum* output)
{
    int shiftBlocks = shift / 64;
    int remaningToShiftBits = shift % 64;
    int desLen = shiftBlocks;

    uint64_t* pDesBlocks = output->blocks;
    for (int i = 0; i < shiftBlocks; ++i)
    {
        // If blocks shifted, we should fill the corresponding blocks with zero.
        *pDesBlocks++ = 0;
    }

    if (remaningToShiftBits == 0)
    {
        // We shift 64 * n (n >= 1) bits. No remaining bits.
        *pDesBlocks = input;
    }
    else
    {
        // We have remaining bits to shift, extend the block.
        ++desLen;

        // Extract the high position bits which would be shifted out of range.
        uint64_t highPositionBits = input >> (64 - remaningToShiftBits);

        // Shift the input. The result should be stored to current block.
        *pDesBlocks = input << remaningToShiftBits;
        if (highPositionBits != 0)
        {
            // If the high position bits is not 0, we should store them to next block.
            *++pDesBlocks = highPositionBits;
            
            // Extend the length accordingly.
            ++desLen;
        }
    }

    output->len = desLen;
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
        shortShiftLeft(realMantissa, realExponent, &numerator);
        denominator.len = 1;
        denominator.blocks[0] = 1;
    }
    else
    {
        // value = (realMantissa * 2^realExponent) / (1)
        //       = (realMantissa / 2^(-realExponent)
        numerator.len = 1;
        numerator.blocks[0] = realMantissa;
        shortShiftLeft(1, -realExponent, &denominator);
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