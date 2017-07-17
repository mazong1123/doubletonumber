#include "gmock/gmock.h"
#include "doubletonumber.h"

class DoubleToNumberTestFixture : public::testing::Test
{
public:
    void assertResult(const NUMBER& expected, const std::wstring& expectedDigits, const NUMBER& actual)
    {
        EXPECT_EQ(expected.precision, actual.precision);
        EXPECT_EQ(expected.scale, actual.scale);
        EXPECT_EQ(expected.sign, actual.sign);
        EXPECT_EQ(expectedDigits, std::wstring(actual.digits));
    }

protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DoubleToNumberTestFixture, DoubleMinValueRoundTripTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 308;
    expected.sign = 1;

    // Act
    NUMBER actual;
    DoubleToNumber(-1.7976931348623157E+308, 17, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"17976931348623157", actual);
}

TEST_F(DoubleToNumberTestFixture, DoubleMaxValueRoundTripTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 308;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(1.7976931348623157e+308, 17, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"17976931348623157", actual);
}

TEST_F(DoubleToNumberTestFixture, BigNumberRoundTripTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 28;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(7.9228162514264338E+28, 17, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"79228162514264338", actual);
}

TEST_F(DoubleToNumberTestFixture, SmallNumberGreaterThanOneRoundTripTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 1;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(70.9228162514264339123, 17, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"7092281625142644", actual);
}

TEST_F(DoubleToNumberTestFixture, SmallNumberGreaterThanOneNormalTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 15;
    expected.scale = 1;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(70.9228162514264339123, 15, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"709228162514264", actual);
}

TEST_F(DoubleToNumberTestFixture, RoundingTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 0;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(3.1415926535897937784612345, 17, &actual);

    NUMBER actual2;
    DoubleToNumber(3.1415926535897937884612345, 17, &actual2);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"31415926535897936", actual);
    DoubleToNumberTestFixture::assertResult(expected, L"31415926535897940", actual2);
}

TEST_F(DoubleToNumberTestFixture, ComplexRoundingTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 19;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(29999999999999792458.0, 17, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"29999999999999791", actual);
}

TEST_F(DoubleToNumberTestFixture, TrailingNinesRoundTripTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 3;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(1000.4999999999999999999, 17, &actual);

    NUMBER actual2;
    DoubleToNumber(1000.9999999999999999999, 17, &actual2);

    NUMBER actual3;
    DoubleToNumber(999.99999999999999999999, 17, &actual3);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"10005000000000000", actual);
    DoubleToNumberTestFixture::assertResult(expected, L"10010000000000000", actual2);
    DoubleToNumberTestFixture::assertResult(expected, L"10000000000000000", actual3);
}

TEST_F(DoubleToNumberTestFixture, SmallestAbsoluteValueNormalTest)
{
    // NOTE: .NET Core's double.ToString("R") has a logic:
    // Try to convert by precision 15, if the value can be converted back to double,
    // we stop. Otherwise we try precision 17.
    //
    // For this smallest value, the string converted by precision 15 is good enough
    // to be converted back to its original double, so double.ToString("R") will use
    // precision 15 result. So here we just test the precision 15 result.

    // Prepare
    NUMBER expected;
    expected.precision = 15;
    expected.scale = -324;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(pow(0.5, 1074), 15, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"494065645841247", actual);
}

TEST_F(DoubleToNumberTestFixture, SpecialRoundTripTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = -1;
    expected.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(0.84551240822557006, 17, &actual);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"84551240822557006", actual);
}

TEST_F(DoubleToNumberTestFixture, TrailingZeroTest)
{
    // Prepare
    NUMBER expected;
    expected.precision = 17;
    expected.scale = 0;
    expected.sign = 0;

    NUMBER expected2;
    expected2.precision = 17;
    expected2.scale = 1;
    expected2.sign = 0;

    // Act
    NUMBER actual;
    DoubleToNumber(1.0, 17, &actual);

    NUMBER actual2;
    DoubleToNumber(10.0, 17, &actual2);

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"10000000000000000", actual);
    DoubleToNumberTestFixture::assertResult(expected2, L"10000000000000000", actual2);
}