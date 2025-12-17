#include "gtest/gtest.h"
#include "Utils.h"
#include "rapidjson/document.h"
#include <string>
#include <vector>
#include <map>
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

    EXPECT_TRUE(any_value_changed((uint8_t)1, (uint8_t)2));
    EXPECT_FALSE(any_value_changed((uint8_t)1, (uint8_t)1));

    EXPECT_TRUE(any_value_changed((int8_t)1, (int8_t)2));
    EXPECT_FALSE(any_value_changed((int8_t)1, (int8_t)1));

    EXPECT_TRUE(any_value_changed((uint16_t)1, (uint16_t)2));
    EXPECT_FALSE(any_value_changed((uint16_t)1, (uint16_t)1));

    EXPECT_TRUE(any_value_changed((int16_t)1, (int16_t)2));
    EXPECT_FALSE(any_value_changed((int16_t)1, (int16_t)1));

    EXPECT_TRUE(any_value_changed((uint32_t)1, (uint32_t)2));
    EXPECT_FALSE(any_value_changed((uint32_t)1, (uint32_t)1));

    EXPECT_TRUE(any_value_changed((int32_t)1, (int32_t)2));
    EXPECT_FALSE(any_value_changed((int32_t)1, (int32_t)1));

    EXPECT_TRUE(any_value_changed((uint64_t)1, (uint64_t)2));
    EXPECT_FALSE(any_value_changed((uint64_t)1, (uint64_t)1));
    EXPECT_TRUE(any_value_changed((int64_t)1, (int64_t)2));
    EXPECT_FALSE(any_value_changed((int64_t)1, (int64_t)1));
    

    EXPECT_TRUE(any_value_changed((float)1, (float)2));
    EXPECT_FALSE(any_value_changed((float)1, (float)1));
    EXPECT_TRUE(any_value_changed((double)1, (double)2));
    EXPECT_FALSE(any_value_changed((double)1, (double)1));


    EXPECT_TRUE(any_value_changed(1, 2));
    EXPECT_FALSE(any_value_changed(1, 1));
    EXPECT_TRUE(any_value_changed(std::string("hello"), std::string("world")));
    EXPECT_FALSE(any_value_changed(std::string("hello"), std::string("hello")));
    EXPECT_TRUE(any_value_changed(true, false));
    EXPECT_FALSE(any_value_changed(true, true));
    EXPECT_TRUE(any_value_changed(1, 1.0)); // Different types

    // Test with shared_ptr<rapidjson::Document>
    auto doc1 = std::make_shared<rapidjson::Document>();
    doc1->Parse(R"({"key":"value"})");
    
    auto doc2 = std::make_shared<rapidjson::Document>();
    doc2->Parse(R"({"key":"value"})");

    auto doc3 = std::make_shared<rapidjson::Document>();
    doc3->Parse(R"({"key":"different_value"})");

    std::shared_ptr<rapidjson::Document> null_doc_ptr;

    EXPECT_FALSE(any_value_changed(doc1, doc1)); // Same object
    EXPECT_FALSE(any_value_changed(doc1, doc2)); // Different objects, same content
    EXPECT_TRUE(any_value_changed(doc1, doc3)); // Different content
    EXPECT_TRUE(any_value_changed(doc1, null_doc_ptr)); // One is null

    // Test with unsupported type
    struct UnsupportedType {};
    UnsupportedType ut1, ut2;
    EXPECT_FALSE(any_value_changed(ut1, ut2));

}

TEST(UtilsTest, AnyToString) {
    // Basic types
    EXPECT_EQ(any_to_string(123), "123");
    EXPECT_EQ(any_to_string(123.456), "123.456000"); // Note: double to string conversion format
    EXPECT_EQ(any_to_string(true), "true");
    EXPECT_EQ(any_to_string(false), "false");
    EXPECT_EQ(any_to_string(std::string("hello")), "hello");
    EXPECT_EQ(any_to_string("world"), "world");
    EXPECT_EQ(any_to_string(std::any()), "N/A");

    // rapidjson::Document
    auto doc = std::make_shared<rapidjson::Document>();
    doc->Parse(R"({"key":"value"})");
    EXPECT_EQ(any_to_string(doc), R"({"key":"value"})");

    // shared_ptr<rapidjson::Document>
    auto doc_ptr = std::make_shared<rapidjson::Document>();
    doc_ptr->Parse(R"({"ptr_key":"ptr_value"})");
    EXPECT_EQ(any_to_string(doc_ptr), R"({"ptr_key":"ptr_value"})");

    // shared_ptr<rapidjson::Document> (null)
    std::shared_ptr<rapidjson::Document> null_doc_ptr;
    EXPECT_EQ(any_to_string(null_doc_ptr), "null");

    // map<string, any>
    std::map<std::string, std::any> test_map;
    test_map["a"] = 1;
    test_map["b"] = std::string("c");
    // The order of elements in JSON from a map is not guaranteed, so we check for members.
    std::string map_str = any_to_string(test_map);
    EXPECT_NE(map_str.find(R"("a":1)"), std::string::npos);
    EXPECT_NE(map_str.find(R"("b":"c")"), std::string::npos);

    // vector<map<string, any>>
    std::vector<std::map<std::string, std::any>> test_vec;
    test_vec.push_back({{"x", 1.5}});
    EXPECT_EQ(any_to_string(test_vec), R"([{"x":1.5}])");

    // Unsupported type
    struct UnsupportedType {};
    UnsupportedType ut;
    // The exact name depends on the compiler, so we check for the prefix.
    EXPECT_TRUE(any_to_string(ut).rfind("Unsupported type: ", 0) == 0);
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

class AnyToJsonValueTest : public ::testing::Test {
protected:
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
};

TEST_F(AnyToJsonValueTest, HandlesNull) {
    rapidjson::Value v = any_to_json_value(std::any(), allocator);
    EXPECT_TRUE(v.IsNull());
}

TEST_F(AnyToJsonValueTest, HandlesString) {
    rapidjson::Value v = any_to_json_value(std::string("hello"), allocator);
    EXPECT_TRUE(v.IsString());
    EXPECT_STREQ(v.GetString(), "hello");
}

TEST_F(AnyToJsonValueTest, HandlesConstChar) {
    rapidjson::Value v = any_to_json_value("world", allocator);
    EXPECT_TRUE(v.IsString());
    EXPECT_STREQ(v.GetString(), "world");
}

TEST_F(AnyToJsonValueTest, HandlesBool) {
    rapidjson::Value v_true = any_to_json_value(true, allocator);
    EXPECT_TRUE(v_true.IsBool());
    EXPECT_TRUE(v_true.GetBool());

    rapidjson::Value v_false = any_to_json_value(false, allocator);
    EXPECT_TRUE(v_false.IsBool());
    EXPECT_FALSE(v_false.GetBool());
}

TEST_F(AnyToJsonValueTest, HandlesIntegerTypes) {
    rapidjson::Value v_u8 = any_to_json_value((uint8_t)10, allocator);
    EXPECT_TRUE(v_u8.IsUint());
    EXPECT_EQ(v_u8.GetUint(), 10);

    rapidjson::Value v_i8 = any_to_json_value((int8_t)-100, allocator);
    EXPECT_TRUE(v_i8.IsInt());
    EXPECT_EQ(v_i8.GetInt(), -100);

    rapidjson::Value v_u16 = any_to_json_value((uint16_t)10, allocator);
    EXPECT_TRUE(v_u16.IsUint());
    EXPECT_EQ(v_u16.GetUint(), 10);

    rapidjson::Value v_i16 = any_to_json_value((int16_t)-100, allocator);
    EXPECT_TRUE(v_i16.IsInt());
    EXPECT_EQ(v_i16.GetInt(), -100);

    rapidjson::Value v_u32 = any_to_json_value((uint32_t)10, allocator);
    EXPECT_TRUE(v_u32.IsUint());
    EXPECT_EQ(v_u32.GetUint(), 10);

    rapidjson::Value v_i32 = any_to_json_value((int32_t)-100, allocator);
    EXPECT_TRUE(v_i32.IsInt());
    EXPECT_EQ(v_i32.GetInt(), -100);

    rapidjson::Value v_u64 = any_to_json_value((uint64_t)1234567890123, allocator);
    EXPECT_TRUE(v_u64.IsUint64());
    EXPECT_EQ(v_u64.GetUint64(), 1234567890123);

    rapidjson::Value v_i64 = any_to_json_value((int64_t)-1234567890123, allocator);
    EXPECT_TRUE(v_i64.IsInt64());
    EXPECT_EQ(v_i64.GetInt64(), -1234567890123);
}

TEST_F(AnyToJsonValueTest, HandlesFloatingPointTypes) {
    rapidjson::Value v_float = any_to_json_value(12.34f, allocator);
    EXPECT_TRUE(v_float.IsFloat());
    EXPECT_FLOAT_EQ(v_float.GetFloat(), 12.34f);

    rapidjson::Value v_double = any_to_json_value(56.78, allocator);
    EXPECT_TRUE(v_double.IsDouble());
    EXPECT_DOUBLE_EQ(v_double.GetDouble(), 56.78);
}

TEST_F(AnyToJsonValueTest, HandlesMap) {
    std::map<std::string, std::any> test_map;
    test_map["key1"] = std::string("value1");
    test_map["key2"] = 123;
    test_map["key3"] = true;

    rapidjson::Value v = any_to_json_value(test_map, allocator);
    EXPECT_TRUE(v.IsObject());
    EXPECT_TRUE(v.HasMember("key1"));
    EXPECT_STREQ(v["key1"].GetString(), "value1");
    EXPECT_TRUE(v.HasMember("key2"));
    EXPECT_EQ(v["key2"].GetInt(), 123);
    EXPECT_TRUE(v.HasMember("key3"));
    EXPECT_TRUE(v["key3"].GetBool());
}

TEST_F(AnyToJsonValueTest, HandlesVectorOfMaps) {
    std::vector<std::map<std::string, std::any>> test_vec;
    std::map<std::string, std::any> map1;
    map1["a"] = 1;
    map1["b"] = std::string("x");
    test_vec.push_back(map1);

    std::map<std::string, std::any> map2;
    map2["c"] = 2.5;
    map2["d"] = false;
    test_vec.push_back(map2);

    rapidjson::Value v = any_to_json_value(test_vec, allocator);
    EXPECT_TRUE(v.IsArray());
    ASSERT_EQ(v.Size(), 2);

    const auto& obj1 = v[0];
    EXPECT_TRUE(obj1.IsObject());
    EXPECT_EQ(obj1["a"].GetInt(), 1);
    EXPECT_STREQ(obj1["b"].GetString(), "x");

    const auto& obj2 = v[1];
    EXPECT_TRUE(obj2.IsObject());
    EXPECT_DOUBLE_EQ(obj2["c"].GetDouble(), 2.5);
    EXPECT_FALSE(obj2["d"].GetBool());
}

TEST_F(AnyToJsonValueTest, HandlesRapidJsonDocument) {
    auto d = std::make_shared<rapidjson::Document>();
    d->Parse(R"({"hello":"world"})");
    
    rapidjson::Value v = any_to_json_value(d, allocator);
    EXPECT_TRUE(v.IsObject());
    EXPECT_TRUE(v.HasMember("hello"));
    EXPECT_STREQ(v["hello"].GetString(), "world");
}

TEST_F(AnyToJsonValueTest, HandlesMapStringString) {
    std::map<std::string, std::string> test_map;
    test_map["key1"] = "value1";
    test_map["key2"] = "value2";

    rapidjson::Value v = any_to_json_value(test_map, allocator);
    EXPECT_TRUE(v.IsObject());
    EXPECT_TRUE(v.HasMember("key1"));
    EXPECT_STREQ(v["key1"].GetString(), "value1");
    EXPECT_TRUE(v.HasMember("key2"));
    EXPECT_STREQ(v["key2"].GetString(), "value2");
}
