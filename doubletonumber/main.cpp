#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>

using namespace std;

unsigned int getExponent(double d)
{
    return (*((unsigned int*)&d + 1) >> 20) & 0x000007FF;
}

uint64_t getMantissa(double d)
{
    return (*(uint64_t*)&d) & 0x000FFFFFFFFFFFFF;
}

uint64_t intPow(uint64_t x, uint64_t p)
{
    if (p == 0)
    {
        return 1;
    }

    if (p == 1)
    {
        return x;
    }

    uint64_t tmp = intPow(x, p / 2);
    if (p % 2 == 0)
    {
        return tmp * tmp;
    }
    else
    {
        return x * tmp * tmp;
    }
}

vector<uint64_t> generate(uint64_t r, uint64_t s, uint64_t mh, uint64_t ml, uint32_t outputBase, bool isLowOK, bool isHighOK)
{
    lldiv_t divResult = lldiv(r * outputBase, s);
    mh = mh * outputBase;
    ml = ml * outputBase;

    uint64_t q = divResult.quot;
    r = divResult.rem;

    vector<uint64_t> d(1, q);

    bool tc1 = isLowOK ? r <= ml : r < ml;
    
    uint64_t high = r + mh;
    bool tc2 = isHighOK ? high >= s : high > s;

    if (!tc1)
    {
        if (!tc2)
        {
            vector<uint64_t> temp = generate(r, s, mh, ml, outputBase, isLowOK, isHighOK);
            d.insert(d.end(), temp.begin(), temp.end());
        }
        else
        {
            d.back()++;
        }

        return d;
    }
    else if (!tc2)
    {
        return d;
    }
    else if (r * 2 < s)
    {
        return d;
    }
    else
    {
        d.back()++;
        return d;
    }

}

vector<uint64_t> scale(uint64_t r, uint64_t s, uint64_t mh, uint64_t ml, uint64_t k, uint32_t outputBase, bool isLowOK, bool isHighOK)
{
    uint64_t temp = r + mh;
    bool cr = isHighOK ? temp >= s : temp > s;
    if (cr)
    {
        return scale(r, s * outputBase, mh, ml, k + 1, outputBase, isLowOK, isHighOK);
    }

    temp = (r + mh) * outputBase;
    cr = isHighOK ? temp < s : temp <= s;
    if (cr)
    {
        return scale(r * outputBase, s, mh * outputBase, ml * outputBase, k - 1, outputBase, isLowOK, isHighOK);
    }

    vector<uint64_t> res(1, k);
    vector<uint64_t> tr = generate(r, s, mh, ml, outputBase, isLowOK, isHighOK);

    res.insert(res.end(), tr.begin(), tr.end());

    return res;
}

vector<uint64_t> floatNumToDigits(double v, uint64_t f, int e, int minExp, int p, uint32_t inputBase, uint32_t outputBase)
{
    // f is even: isRound = true. f is odd: isRound = false.
    bool isRound = !(f & 1);
    if (e >= 0)
    {
        if (f != intPow(inputBase, p - 1))
        {
            uint64_t be = intPow(inputBase, e);
            
            return scale(f * be * 2, 2, be, be, 0, outputBase, isRound, isRound);
        }
        else
        {
            uint64_t be = intPow(inputBase, e);
            uint64_t be1 = be * inputBase;

            return scale(f * be1 * 2, inputBase * 2, be1, be, 0, outputBase, isRound, isRound);
        }
    }
    else if (e == minExp || f != intPow(inputBase, p - 1))
    {
        return scale(f * 2, intPow(inputBase, -e) * 2, 1, 1, 0, outputBase, isRound, isRound);
    }
    else
    {
        return scale(f * inputBase * 2, intPow(inputBase, 1 - e) * 2, inputBase, 1, 0, outputBase, isRound, isRound);
    }
}

int main()
{
    //vector<uint64_t> res = generate(22042, 200000, 1, 1, 10, false, false);
    //vector<uint64_t> res = scale(22042, 20000, 1, 1, 0, 10, false, false);
    vector<uint64_t> res = floatNumToDigits(1.1021, 11021, -4, -308, 17, 10, 10);

    for (int i = 0; i < res.size(); ++i)
    {
        cout << res[i] << endl;
    }
    
    return 0;
}