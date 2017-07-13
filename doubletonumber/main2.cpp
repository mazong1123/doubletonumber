#include <math.h>

int main()
{
    const int c_maxDigits = 256;

    // input binary floating-point number to convert from
    double value = 122.5;   // the number to convert

                            // output decimal representation to convert to
    char   decimalDigits[c_maxDigits]; // buffer to put the decimal representation in
    int    numDigits = 0;              // this will be set to the number of digits in the buffer
    int    firstDigitExponent = 0;     // this will be set to the the base 10 exponent of the first
                                       //  digit in the buffer   

                                       // Compute the first digit's exponent in the equation digit*10^exponent
                                       // (e.g. 122.5 would compute a 2 because its first digit is in the hundreds place) 
    firstDigitExponent = (int)floor(log10(value));

    // Scale the input value such that the first digit is in the ones place
    // (e.g. 122.5 would become 1.225).
    value = value / pow(10, firstDigitExponent);

    // while there is a non-zero value to print and we have room in the buffer
    while (value > 0.0 && numDigits < c_maxDigits)
    {
        // Output the current digit.
        double digit = floor(value);
        decimalDigits[numDigits] = '0' + (char)digit; // convert to an ASCII character
        ++numDigits;

        // Compute the remainder by subtracting the current digit
        // (e.g. 1.225 would becom 0.225)
        value -= digit;

        // Scale the next digit into the ones place.
        // (e.g. 0.225 would becom 2.25)
        value *= 10.0;
    }

    return 0;
}