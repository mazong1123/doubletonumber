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

TEST_F(DoubleToNumberTestFixture, TrailingNinesTest)
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

    // Assert
    DoubleToNumberTestFixture::assertResult(expected, L"10005000000000000", actual);
    DoubleToNumberTestFixture::assertResult(expected, L"10010000000000000", actual2);
}