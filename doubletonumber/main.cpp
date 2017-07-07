#include <cmath>
#include <cstdint>

unsigned int getExponent(double d)
{
    return (*((unsigned int*)&d + 1) >> 20) & 0x000007FF;
}

uint64_t getMantissa(double d)
{
    return (*(uint64_t*)&d) & 0x000FFFFFFFFFFFFF;
}

int main()
{
    return 0;
}