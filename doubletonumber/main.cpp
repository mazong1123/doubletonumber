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
    }
    else if (!tc2)
    {
        return d;
    }
    else if (r * 2 < s)
    {
        return d;
    }

    d.back()++;

    return d;
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
int main()
{
    //vector<uint64_t> res = generate(11, 2, 3, 4, 10, true, true);
    vector<uint64_t> res = scale(15000, 500, 10000, 60, 0, 10, false, false);

    for (int i = 0; i < res.size(); ++i)
    {
        cout << res[i] << endl;
    }
    
    return 0;
}