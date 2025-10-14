#include "gtest/gtest.h"
#include "Utils.h"
#include <any>

TEST(UtilsTest, Trim) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("hello  "), "hello");
    EXPECT_EQ(trim("  hello"), "hello");
    EXPECT_EQ(trim("hello"), "hello");
    EXPECT_EQ(trim("  "), "");
    EXPECT_EQ(trim(""), "");
}

TEST(UtilsTest, UrlToPath) {
    EXPECT_EQ(UrlToPath("viss/Vehicle/Speed"), "viss.Vehicle.Speed");
    EXPECT_EQ(UrlToPath("Vehicle/Speed"), "Vehicle.Speed");
    EXPECT_EQ(UrlToPath("Vehicle"), "Vehicle");
}

TEST(UtilsTest, AnyValueChanged) {
    EXPECT_TRUE(any_value_changed(1, 2));
    EXPECT_FALSE(any_value_changed(1, 1));
    EXPECT_TRUE(any_value_changed(std::string("hello"), std::string("world")));
    EXPECT_FALSE(any_value_changed(std::string("hello"), std::string("hello")));
    EXPECT_TRUE(any_value_changed(true, false));
    EXPECT_FALSE(any_value_changed(true, true));
    EXPECT_TRUE(any_value_changed(1, 1.0)); // Different types
}

TEST(UtilsTest, AnyToString) {
    EXPECT_EQ(any_to_string(123), "123");
    EXPECT_EQ(any_to_string(123.456), "123.456000"); // Note: double to string conversion format
    EXPECT_EQ(any_to_string(true), "true");
    EXPECT_EQ(any_to_string(std::string("hello")), "hello");
}

TEST(UtilsTest, FormatTimestamp) {
    // Test a known timestamp
    // 2023-10-27 10:00:00 UTC = 1698397200000 ms
    long long timestamp = 1698397200000;
    EXPECT_EQ(formatTimestamp(timestamp), "2023-10-27T09:00:00.000Z"); // Adjusted for GMT
}


TEST(UtilsTest, ParseTimePeriod) {
    // Test with all components
    EXPECT_EQ(parseTimePeriod("P1DT2H3M4S"), (1 * 24 * 60 * 60 + 2 * 60 * 60 + 3 * 60 + 4) * 1000LL);

    // Test with only days
    EXPECT_EQ(parseTimePeriod("P3D"), 3 * 24 * 60 * 60 * 1000LL);

    // Test with only time components
    EXPECT_EQ(parseTimePeriod("PT5H10M30S"), (5 * 60 * 60 + 10 * 60 + 30) * 1000LL);

    // Test with only hours and seconds
    EXPECT_EQ(parseTimePeriod("PT1H1S"), (1 * 60 * 60 + 1) * 1000LL);

    // Test with only minutes
    EXPECT_EQ(parseTimePeriod("PT20M"), 20 * 60 * 1000LL);

    // Test with zero values
    EXPECT_EQ(parseTimePeriod("P0DT0H0M0S"), 0);

    // Test with invalid format
    EXPECT_EQ(parseTimePeriod(""), 0);
    EXPECT_EQ(parseTimePeriod("invalid"), 0);
    EXPECT_EQ(parseTimePeriod("P1D2H3M4S"), 0); // Missing T
    EXPECT_EQ(parseTimePeriod("PT10S1H"), 0); // Invalid order
    EXPECT_EQ(parseTimePeriod("P1M"), 0); // Invalid unit in date part
    EXPECT_EQ(parseTimePeriod("PT1D"), 0); // Invalid unit in time part
}
