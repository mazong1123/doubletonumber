#include "gmock/gmock.h"
#include "doubletonumber.h"

class DoubleToNumberTestFixture : public::testing::Test
{
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
    int expectedPrecision = 17;
    int expectedScale = 308;
    int expectedSign = 1;

    std::wstring expectedDigits = L"17976931348623157";

    // Act
    NUMBER actual;
    DoubleToNumber(-1.7976931348623157E+308, 17, &actual);

    // Assert
    EXPECT_EQ(expectedPrecision, actual.precision);
    EXPECT_EQ(expectedScale, actual.scale);
    EXPECT_EQ(expectedSign, actual.sign);
    EXPECT_EQ(expectedDigits, std::wstring(actual.digits));
}

TEST_F(DoubleToNumberTestFixture, BigNumberRoundTripTest)
{
    // Prepare
    int expectedPrecision = 17;
    int expectedScale = 28;
    int expectedSign = 0;

    std::wstring expectedDigits = L"79228162514264338";

    // Act
    NUMBER actual;
    DoubleToNumber(7.9228162514264338E+28, 17, &actual);

    // Assert
    EXPECT_EQ(expectedPrecision, actual.precision);
    EXPECT_EQ(expectedScale, actual.scale);
    EXPECT_EQ(expectedSign, actual.sign);
    EXPECT_EQ(expectedDigits, std::wstring(actual.digits));
}

TEST_F(DoubleToNumberTestFixture, SmallNumberGreaterThanOneRoundTripTest)
{
    // Prepare
    int expectedPrecision = 17;
    int expectedScale = 1;
    int expectedSign = 0;

    std::wstring expectedDigits = L"7092281625142644";

    // Act
    NUMBER actual;
    DoubleToNumber(70.9228162514264339123, 17, &actual);

    // Assert
    EXPECT_EQ(expectedPrecision, actual.precision);
    EXPECT_EQ(expectedScale, actual.scale);
    EXPECT_EQ(expectedSign, actual.sign);
    EXPECT_EQ(expectedDigits, std::wstring(actual.digits));
}

TEST_F(DoubleToNumberTestFixture, SmallNumberGreaterThanOneNormalTest)
{
    // Prepare
    int expectedPrecision = 15;
    int expectedScale = 1;
    int expectedSign = 0;

    std::wstring expectedDigits = L"709228162514264";

    // Act
    NUMBER actual;
    DoubleToNumber(70.9228162514264339123, 15, &actual);

    // Assert
    EXPECT_EQ(expectedPrecision, actual.precision);
    EXPECT_EQ(expectedScale, actual.scale);
    EXPECT_EQ(expectedSign, actual.sign);
    EXPECT_EQ(expectedDigits, std::wstring(actual.digits));
}