#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>

using namespace std;

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

    char* lpStartOfReturnBuffer = new char[348];

    char TempBuffer[348];

    *dec = *sign = 0;

    if (value < 0.0)
    {
        *sign = 1;
    }

    {
        // we have issue #10290 tracking fixing the sign of 0.0 across the platforms
        if (value == 0.0)
        {
            for (int j = 0; j < count; j++)
            {
                lpStartOfReturnBuffer[j] = '0';
            }
            lpStartOfReturnBuffer[count] = '\0';
            goto done;
        }

        int tempBufferLength = snprintf(TempBuffer, 348, "%.40e", value);
        
        //
        // Calculate the exponent value
        //

        int exponentIndex = strrchr(TempBuffer, 'e') - TempBuffer;
        
        int i = exponentIndex + 1;
        int exponentSign = 1;
        if (TempBuffer[i] == '-')
        {
            exponentSign = -1;
            i++;
        }
        else if (TempBuffer[i] == '+')
        {
            i++;
        }

        int exponentValue = 0;
        while (i < tempBufferLength)
        {
            _ASSERTE(TempBuffer[i] >= '0' && TempBuffer[i] <= '9');
            exponentValue = exponentValue * 10 + ((unsigned char)TempBuffer[i] - (unsigned char) '0');
            i++;
        }
        exponentValue *= exponentSign;

        //
        // Determine decimal location.
        // 

        if (exponentValue == 0)
        {
            *dec = 1;
        }
        else
        {
            *dec = exponentValue + 1;
        }

        //
        // Copy the string from the temp buffer upto precision characters, removing the sign, and decimal as required.
        // 

        i = 0;
        int mantissaIndex = 0;
        while (i < count && mantissaIndex < exponentIndex)
        {
            if (TempBuffer[mantissaIndex] >= '0' && TempBuffer[mantissaIndex] <= '9')
            {
                lpStartOfReturnBuffer[i] = TempBuffer[mantissaIndex];
                i++;
            }
            mantissaIndex++;
        }

        while (i < count)
        {
            lpStartOfReturnBuffer[i] = '0'; // append zeros as needed
            i++;
        }

        lpStartOfReturnBuffer[i] = '\0';

        //
        // Round if needed
        //

        if (mantissaIndex >= exponentIndex || TempBuffer[mantissaIndex] < '5')
        {
            goto done;
        }

        i = count - 1;
        while (lpStartOfReturnBuffer[i] == '9' && i > 0)
        {
            lpStartOfReturnBuffer[i] = '0';
            i--;
        }

        if (i == 0 && lpStartOfReturnBuffer[i] == '9')
        {
            lpStartOfReturnBuffer[i] = '1';
            (*dec)++;
        }
        else
        {
            lpStartOfReturnBuffer[i]++;
        }
    }

done:

    return lpStartOfReturnBuffer;
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
    DoubleToNumber(7.9228162514264338e+28, 15, &number);
    return 0;
}