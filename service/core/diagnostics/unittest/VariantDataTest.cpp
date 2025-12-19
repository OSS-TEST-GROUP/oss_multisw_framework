#include <gtest/gtest.h>

#include "diagnostic/VariantData.cpp"
#include "mock/MockLogger.hpp"

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class VariantDataTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        ins = new VariantData();
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        delete ins;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    VariantData* ins;
};

TEST_F(VariantDataTest, SetName_TC001) {
    ins->SetName("Test123");
    EXPECT_EQ("Test123", ins->name_);
}

TEST_F(VariantDataTest, GetName_TC001) {
    ins->name_ = "HelloWorld";
    EXPECT_EQ("HelloWorld", ins->GetName());
}

TEST_F(VariantDataTest, SetValue_TC001_uint64) {
    ins->SetValue("key1", 123ULL);
    EXPECT_EQ(std::type_index(typeid(uint64_t)), ins->GetValueType("key1"));
    EXPECT_EQ(123ULL, std::get<uint64_t>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_uint32) {
    ins->SetValue("key1", 123UL);
    EXPECT_EQ(std::type_index(typeid(uint64_t)), ins->GetValueType("key1"));
    EXPECT_EQ(123UL, std::get<uint64_t>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_int64) {
    ins->SetValue("key1", -123LL);
    EXPECT_EQ(std::type_index(typeid(int64_t)), ins->GetValueType("key1"));
    EXPECT_EQ(-123LL, std::get<int64_t>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_int32) {
    ins->SetValue("key1", -123L);
    EXPECT_EQ(std::type_index(typeid(int64_t)), ins->GetValueType("key1"));
    EXPECT_EQ(-123L, std::get<int64_t>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_double) {
    ins->SetValue("key1", 3.14);
    EXPECT_EQ(std::type_index(typeid(double)), ins->GetValueType("key1"));
    EXPECT_EQ(3.14, std::get<double>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_float) {
    ins->SetValue("key1", 3.14);
    EXPECT_EQ(std::type_index(typeid(double)), ins->GetValueType("key1"));
    EXPECT_EQ(3.14, std::get<double>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_bool) {
    ins->SetValue("key1", true);
    EXPECT_EQ(std::type_index(typeid(bool)), ins->GetValueType("key1"));
    EXPECT_EQ(true, std::get<bool>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_string) {
    ins->SetValue("key1", "hello");
    EXPECT_EQ(std::type_index(typeid(std::string)), ins->GetValueType("key1"));
    EXPECT_EQ("hello", std::get<std::string>(ins->members_["key1"]));
}

TEST_F(VariantDataTest, SetValue_TC001_variantdata) {
    VariantData value1;
    value1.SetName("Hello");
    ins->SetValue("key1", value1);
    EXPECT_EQ(std::type_index(typeid(VariantData)), ins->GetValueType("key1"));
    EXPECT_EQ("Hello", std::get<VariantData>(ins->members_["key1"]).GetName());
}

TEST_F(VariantDataTest, SetValue_TC001_vector_variantdata) {
    std::vector<VariantData> vec1;
    VariantData value1;
    value1.SetName("Hello");
    vec1.push_back(value1);
    ins->SetValue("key1", vec1);
    EXPECT_EQ(std::type_index(typeid(std::vector<VariantData>)), ins->GetValueType("key1"));
    EXPECT_EQ("Hello", std::get<std::vector<VariantData>>(ins->members_["key1"])[0].GetName());
}

TEST_F(VariantDataTest, GetValue_TC001_uint64) {
    ins->members_["key1"] = 1234ULL;

    uint64_t ret = ins->GetValue<uint64_t>("key1");
    EXPECT_EQ(1234ULL, ret);
}

TEST_F(VariantDataTest, GetValue_TC001_int64) {
    ins->members_["key1"] = -1234LL;

    int64_t ret = ins->GetValue<int64_t>("key1");
    EXPECT_EQ(-1234LL, ret);
}

TEST_F(VariantDataTest, GetValue_TC001_bool) {
    ins->members_["key1"] = true;

    bool ret = ins->GetValue<bool>("key1");
    EXPECT_EQ(true, ret);
}

TEST_F(VariantDataTest, GetValue_TC001_string) {
    ins->members_["key1"] = "hello";

    std::string ret = ins->GetValue<std::string>("key1");
    EXPECT_EQ("hello", ret);
}

TEST_F(VariantDataTest, GetValue_TC001_variant) {
    VariantData value1;
    value1.SetName("Hello");
    ins->members_["key1"] = value1;

    VariantData ret = ins->GetValue<VariantData>("key1");
    EXPECT_EQ(value1.GetName(), ret.GetName());
}

TEST_F(VariantDataTest, GetValue_TC001_vector_variantdata) {
    std::vector<VariantData> vec1;
    VariantData value1;
    value1.SetName("Hello");
    vec1.push_back(value1);
    ins->members_["key1"] = vec1;

    std::vector<VariantData> ret = ins->GetValue<std::vector<VariantData>>("key1");
    EXPECT_EQ("Hello", ret[0].GetName());
}

TEST_F(VariantDataTest, GetValueType_TC001) {
    ins->members_["key1"] = 3.14;

    EXPECT_EQ(std::type_index(typeid(double)), ins->GetValueType("key1"));
}

TEST_F(VariantDataTest, GetChild_TC001_Valid) {
    VariantData child;
    child.SetName("ChildNode");
    ins->SetValue("nested", child);

    auto result = ins->GetChild("nested");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ("ChildNode", result->GetName());
}

TEST_F(VariantDataTest, GetChild_TC002_InvalidKey) {
    auto result = ins->GetChild("no_such_key");
    EXPECT_EQ(result, nullptr);
}

TEST_F(VariantDataTest, GetInt64_TC001_Valid) {
    ins->SetValue("val", -123L);
    int64_t out;
    EXPECT_TRUE(ins->GetInt64("val", out));
    EXPECT_EQ(out, -123L);
}

TEST_F(VariantDataTest, GetInt64_TC002_InvalidType) {
    ins->SetValue("val", std::string("oops"));
    int64_t out;
    EXPECT_FALSE(ins->GetInt64("val", out));
}

TEST_F(VariantDataTest, GetUInt64_TC001_Valid) {
    ins->SetValue("val", 123UL);
    uint64_t out;
    EXPECT_TRUE(ins->GetUInt64("val", out));
    EXPECT_EQ(out, 123UL);
}

TEST_F(VariantDataTest, GetDouble_TC001_FromInt) {
    ins->SetValue("val", 42);
    double out;
    EXPECT_TRUE(ins->GetDouble("val", out));
    EXPECT_DOUBLE_EQ(out, 42.0);
}

TEST_F(VariantDataTest, GetString_TC001) {
    ins->SetValue("val", std::string("hello"));
    std::string out;
    EXPECT_TRUE(ins->GetString("val", out));
    EXPECT_EQ(out, "hello");
}

TEST_F(VariantDataTest, GetChildFromList_TC001_Valid) {
    VariantData child;
    child.SetName("Child1");
    std::vector<VariantData> children = { child };
    ins->SetValue("listKey", children);

    VariantData* result = ins->GetChildFromList("listKey", 0);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ("Child1", result->GetName());
}

TEST_F(VariantDataTest, GetInt64FromList_TC001_Valid) {
    std::vector<int64_t> values = { -456LL };
    ins->SetValue("listKey", values);

    int64_t out;
    EXPECT_TRUE(ins->GetInt64FromList("listKey", 0, out));
    EXPECT_EQ(out, -456LL);
}

TEST_F(VariantDataTest, GetUInt64FromList_TC001_Valid) {
    std::vector<uint64_t> values = { 789ULL };
    ins->SetValue("listKey", values);

    uint64_t out;
    EXPECT_TRUE(ins->GetUInt64FromList("listKey", 0, out));
    EXPECT_EQ(out, 789ULL);
}

TEST_F(VariantDataTest, GetDoubleFromList_TC001) {
    std::vector<double> values = { 42.0 };
    ins->SetValue("listKey", values);

    double out;
    EXPECT_TRUE(ins->GetDoubleFromList("listKey", 0, out));
    EXPECT_DOUBLE_EQ(out, 42.0);
}

TEST_F(VariantDataTest, GetStringFromList_TC001_Valid) {
    std::vector<std::string> values = { "abc" };
    ins->SetValue("listKey", values);

    std::string out;
    EXPECT_TRUE(ins->GetStringFromList("listKey", 0, out));
    EXPECT_EQ(out, "abc");
}

TEST_F(VariantDataTest, HasKey_TC001) {
    ins->members_["key1"] = 3.14;

    EXPECT_TRUE(ins->HasKey("key1"));
    EXPECT_FALSE(ins->HasKey("key2"));
}

TEST_F(VariantDataTest, GetKeyList_TC001) {
    ins->members_["key1"] = 3.14;
    ins->members_["key2"] = 3.14;
    ins->members_["key3"] = 3.14;

    std::vector<std::string> ret = ins->GetKeyList();
    EXPECT_TRUE(std::find(ret.cbegin(), ret.cend(), "key1") != ret.end());
    EXPECT_TRUE(std::find(ret.cbegin(), ret.cend(), "key2") != ret.end());
    EXPECT_TRUE(std::find(ret.cbegin(), ret.cend(), "key3") != ret.end());
}

TEST_F(VariantDataTest, GetValueNum_TC001) {
    ins->members_["key1"] = 3.14;
    ins->members_["key2"] = 3.14;
    ins->members_["key3"] = 3.14;
    ins->members_["key4"] = 3.14;
    EXPECT_EQ(4U, ins->GetValueNum());
}

TEST_F(VariantDataTest, GetValueNum_Empty_TC002) {
    EXPECT_EQ(0U, ins->GetValueNum());
}

TEST_F(VariantDataTest, HasKey_NonExistent_TC002) {
    ins->SetValue("existing", 123);
    EXPECT_TRUE(ins->HasKey("existing"));
    EXPECT_FALSE(ins->HasKey("nonexistent"));
    EXPECT_FALSE(ins->HasKey(""));
}

TEST_F(VariantDataTest, GetChild_NonExistent_TC002) {
    VariantData* child = ins->GetChild("nonexistent");
    EXPECT_EQ(nullptr, child);
}

TEST_F(VariantDataTest, GetChild_WrongType_TC003) {
    ins->SetValue("not_child", 123);
    VariantData* child = ins->GetChild("not_child");
    EXPECT_EQ(nullptr, child);
}

TEST_F(VariantDataTest, GetInt64_NonExistent_TC002) {
    int64_t out;
    EXPECT_FALSE(ins->GetInt64("nonexistent", out));
}

TEST_F(VariantDataTest, GetInt64_WrongType_TC003) {
    ins->SetValue("str_key", "hello");
    int64_t out;
    EXPECT_FALSE(ins->GetInt64("str_key", out));
}

TEST_F(VariantDataTest, GetUInt64_NonExistent_TC002) {
    uint64_t out;
    EXPECT_FALSE(ins->GetUInt64("nonexistent", out));
}

TEST_F(VariantDataTest, GetDouble_NonExistent_TC002) {
    double out;
    EXPECT_FALSE(ins->GetDouble("nonexistent", out));
}

TEST_F(VariantDataTest, GetString_NonExistent_TC002) {
    std::string out;
    EXPECT_FALSE(ins->GetString("nonexistent", out));
}

TEST_F(VariantDataTest, GetChildFromList_NonExistent_TC002) {
    VariantData* child = ins->GetChildFromList("nonexistent", 0);
    EXPECT_EQ(nullptr, child);
}

TEST_F(VariantDataTest, GetChildFromList_OutOfBounds_TC003) {
    VariantData child;
    std::vector<VariantData> children = { child };
    ins->SetValue("listKey", children);

    VariantData* result = ins->GetChildFromList("listKey", 10);
    EXPECT_EQ(nullptr, result);
}

TEST_F(VariantDataTest, GetInt64FromList_NonExistent_TC002) {
    int64_t out;
    EXPECT_FALSE(ins->GetInt64FromList("nonexistent", 0, out));
}

TEST_F(VariantDataTest, GetInt64FromList_OutOfBounds_TC003) {
    std::vector<int64_t> values = { 123LL };
    ins->SetValue("listKey", values);

    int64_t out;
    EXPECT_FALSE(ins->GetInt64FromList("listKey", 10, out));
}

TEST_F(VariantDataTest, GetUInt64FromList_NonExistent_TC002) {
    uint64_t out;
    EXPECT_FALSE(ins->GetUInt64FromList("nonexistent", 0, out));
}

TEST_F(VariantDataTest, GetDoubleFromList_NonExistent_TC002) {
    double out;
    EXPECT_FALSE(ins->GetDoubleFromList("nonexistent", 0, out));
}

TEST_F(VariantDataTest, GetStringFromList_NonExistent_TC002) {
    std::string out;
    EXPECT_FALSE(ins->GetStringFromList("nonexistent", 0, out));
}

TEST_F(VariantDataTest, AllIntegerTypes_TC010) {
    ins->SetValue("int8", int8_t(127));
    ins->SetValue("int16", int16_t(32767));
    ins->SetValue("int32", int32_t(2147483647));
    ins->SetValue("uint8", uint8_t(255));
    ins->SetValue("uint16", uint16_t(65535));
    ins->SetValue("uint32", uint32_t(4294967295));
    
    EXPECT_TRUE(ins->HasKey("int8"));
    EXPECT_TRUE(ins->HasKey("int16"));
    EXPECT_TRUE(ins->HasKey("int32"));
    EXPECT_TRUE(ins->HasKey("uint8"));
    EXPECT_TRUE(ins->HasKey("uint16"));
    EXPECT_TRUE(ins->HasKey("uint32"));
}

TEST_F(VariantDataTest, VectorTypes_TC011) {
    std::vector<int32_t> int_vec = {1, 2, 3};
    std::vector<double> double_vec = {1.1, 2.2, 3.3};
    std::vector<std::string> str_vec = {"a", "b", "c"};
    std::vector<bool> bool_vec = {true, false, true};
    
    ins->SetValue("int_vec", int_vec);
    ins->SetValue("double_vec", double_vec);
    ins->SetValue("str_vec", str_vec);
    ins->SetValue("bool_vec", bool_vec);
    
    EXPECT_EQ(4U, ins->GetValueNum());
}

TEST_F(VariantDataTest, GetValueAsVariant_TC012) {
    ins->SetValue("test_key", 42);
    VariantValue vv = ins->GetValueAsVariant("test_key");
    EXPECT_EQ(std::type_index(typeid(int64_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_ValidIndex_TC013) {
    std::vector<int32_t> values = {10, 20, 30};
    ins->SetValue("list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("list", 1);
    EXPECT_EQ(std::type_index(typeid(int32_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_InvalidIndex_TC014) {
    std::vector<int32_t> values = {10, 20};
    ins->SetValue("list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("list", 10);
    // Should have default type
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_NonExistent_TC015) {
    VariantValue vv = ins->GetValueAsVariantFromList("nonexistent", 0);
    // Should have default type
}

TEST_F(VariantDataTest, NestedVariantData_TC016) {
    VariantData child1;
    child1.SetName("child1");
    child1.SetValue("child_key", 123);
    
    VariantData child2;
    child2.SetName("child2");
    child2.SetValue("child_key", 456);
    
    ins->SetValue("nested1", child1);
    ins->SetValue("nested2", child2);
    
    auto* retrieved1 = ins->GetChild("nested1");
    auto* retrieved2 = ins->GetChild("nested2");
    
    ASSERT_NE(nullptr, retrieved1);
    ASSERT_NE(nullptr, retrieved2);
    EXPECT_EQ("child1", retrieved1->GetName());
    EXPECT_EQ("child2", retrieved2->GetName());
}

TEST_F(VariantDataTest, EmptyKeyList_TC017) {
    std::vector<std::string> keys = ins->GetKeyList();
    EXPECT_TRUE(keys.empty());
}

TEST_F(VariantDataTest, LargeKeyList_TC018) {
    for (int i = 0; i < 100; ++i) {
        ins->SetValue("key" + std::to_string(i), i);
    }
    
    std::vector<std::string> keys = ins->GetKeyList();
    EXPECT_EQ(100U, keys.size());
}

TEST_F(VariantDataTest, SpecialCharactersInKeys_TC019) {
    ins->SetValue("key-1_test", 1);
    ins->SetValue("key.2.test", 2);
    ins->SetValue("key@3", 3);
    
    EXPECT_TRUE(ins->HasKey("key-1_test"));
    EXPECT_TRUE(ins->HasKey("key.2.test"));
    EXPECT_TRUE(ins->HasKey("key@3"));
}

TEST_F(VariantDataTest, OverwriteValue_TC020) {
    ins->SetValue("key1", 123);
    ins->SetValue("key1", 456);
    
    int64_t out;
    EXPECT_TRUE(ins->GetInt64("key1", out));
    EXPECT_EQ(456, out);
}

// ============================================================================
// GetValueAsVariantFromList Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetValueAsVariantFromList_WcharVector_TC021) {
    std::vector<wchar_t> values = {L'A', L'B', L'C'};
    ins->SetValue("wchar_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("wchar_list", 1);
    EXPECT_EQ(std::type_index(typeid(wchar_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_Int8Vector_TC022) {
    std::vector<int8_t> values = {1, 2, 3};
    ins->SetValue("int8_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("int8_list", 0);
    EXPECT_EQ(std::type_index(typeid(int8_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_Int16Vector_TC023) {
    std::vector<int16_t> values = {100, 200, 300};
    ins->SetValue("int16_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("int16_list", 1);
    EXPECT_EQ(std::type_index(typeid(int16_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_Int64Vector_TC024) {
    std::vector<int64_t> values = {1000LL, 2000LL, 3000LL};
    ins->SetValue("int64_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("int64_list", 2);
    EXPECT_EQ(std::type_index(typeid(int64_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_UInt8Vector_TC025) {
    std::vector<uint8_t> values = {1, 2, 3};
    ins->SetValue("uint8_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("uint8_list", 0);
    EXPECT_EQ(std::type_index(typeid(uint8_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_UInt16Vector_TC026) {
    std::vector<uint16_t> values = {100, 200, 300};
    ins->SetValue("uint16_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("uint16_list", 1);
    EXPECT_EQ(std::type_index(typeid(uint16_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_UInt32Vector_TC027) {
    std::vector<uint32_t> values = {1000, 2000, 3000};
    ins->SetValue("uint32_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("uint32_list", 0);
    EXPECT_EQ(std::type_index(typeid(uint32_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_UInt64Vector_TC028) {
    std::vector<uint64_t> values = {10000ULL, 20000ULL, 30000ULL};
    ins->SetValue("uint64_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("uint64_list", 1);
    EXPECT_EQ(std::type_index(typeid(uint64_t)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_DoubleVector_TC029) {
    std::vector<double> values = {1.1, 2.2, 3.3};
    ins->SetValue("double_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("double_list", 0);
    EXPECT_EQ(std::type_index(typeid(double)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_BoolVector_TC030) {
    std::vector<bool> values = {true, false, true};
    ins->SetValue("bool_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("bool_list", 1);
    EXPECT_EQ(std::type_index(typeid(bool)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_StringVector_TC031) {
    std::vector<std::string> values = {"hello", "world", "test"};
    ins->SetValue("string_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("string_list", 0);
    EXPECT_EQ(std::type_index(typeid(std::string)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_WstringVector_TC032) {
    std::vector<std::wstring> values = {L"wide", L"string", L"test"};
    ins->SetValue("wstring_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("wstring_list", 1);
    EXPECT_EQ(std::type_index(typeid(std::wstring)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_CharVector_TC033) {
    std::vector<char> values = {'a', 'b', 'c'};
    ins->SetValue("char_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("char_list", 0);
    EXPECT_EQ(std::type_index(typeid(char)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_NonVector_TC034) {
    // Set a non-vector value and try to get from list (should log error)
    ins->SetValue("not_list", 123);
    
    VariantValue vv = ins->GetValueAsVariantFromList("not_list", 0);
    // Should return default VariantValue with void type
    EXPECT_EQ(std::type_index(typeid(void)), vv.type);
}

TEST_F(VariantDataTest, GetValueAsVariantFromList_OutOfBounds_WcharVector_TC035) {
    std::vector<wchar_t> values = {L'A', L'B'};
    ins->SetValue("wchar_list", values);
    
    VariantValue vv = ins->GetValueAsVariantFromList("wchar_list", 10);
    // Type should be set, but value not assigned due to out of bounds
    EXPECT_EQ(std::type_index(typeid(wchar_t)), vv.type);
}

// ============================================================================
// GetUInt64 Branch Coverage Tests  
// ============================================================================

TEST_F(VariantDataTest, GetUInt64_WrongType_TC021) {
    ins->SetValue("str_key", std::string("hello"));
    uint64_t out;
    EXPECT_FALSE(ins->GetUInt64("str_key", out));
}

// ============================================================================
// GetDouble Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetDouble_WrongType_TC021) {
    ins->SetValue("str_key", std::string("hello"));
    double out;
    EXPECT_FALSE(ins->GetDouble("str_key", out));
}

// ============================================================================
// GetBoolean Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetBoolean_Valid_TC021) {
    ins->SetValue("bool_key", true);
    bool out;
    EXPECT_TRUE(ins->GetBoolean("bool_key", out));
    EXPECT_TRUE(out);
}

TEST_F(VariantDataTest, GetBoolean_NonExistent_TC022) {
    bool out;
    EXPECT_FALSE(ins->GetBoolean("nonexistent", out));
}

TEST_F(VariantDataTest, GetBoolean_WrongType_TC023) {
    ins->SetValue("int_key", 123);
    bool out;
    EXPECT_FALSE(ins->GetBoolean("int_key", out));
}

// ============================================================================
// GetString with vector<char> conversion Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetString_FromCharVector_TC021) {
    std::vector<char> char_vec = {'h', 'e', 'l', 'l', 'o', '\0'};
    ins->SetValue("char_vec_key", char_vec);
    
    std::string out;
    EXPECT_TRUE(ins->GetString("char_vec_key", out));
    EXPECT_EQ("hello", out);
}

TEST_F(VariantDataTest, GetString_FromCharVector_NoNull_TC022) {
    std::vector<char> char_vec = {'a', 'b', 'c'};
    ins->SetValue("char_vec_key", char_vec);
    
    std::string out;
    EXPECT_TRUE(ins->GetString("char_vec_key", out));
    EXPECT_EQ("abc", out);
}

TEST_F(VariantDataTest, GetString_WrongType_TC023) {
    ins->SetValue("int_key", 123);
    std::string out;
    EXPECT_FALSE(ins->GetString("int_key", out));
}

// ============================================================================
// GetBooleanFromList Branch Coverage Tests (entirely uncovered)
// ============================================================================

TEST_F(VariantDataTest, GetBooleanFromList_Valid_TC021) {
    std::vector<bool> values = {true, false, true};
    ins->SetValue("bool_list", values);
    
    bool out;
    EXPECT_TRUE(ins->GetBooleanFromList("bool_list", 0, out));
    EXPECT_TRUE(out);
    
    EXPECT_TRUE(ins->GetBooleanFromList("bool_list", 1, out));
    EXPECT_FALSE(out);
}

TEST_F(VariantDataTest, GetBooleanFromList_NonExistent_TC022) {
    bool out;
    EXPECT_FALSE(ins->GetBooleanFromList("nonexistent", 0, out));
}

TEST_F(VariantDataTest, GetBooleanFromList_OutOfBounds_TC023) {
    std::vector<bool> values = {true, false};
    ins->SetValue("bool_list", values);
    
    bool out;
    EXPECT_FALSE(ins->GetBooleanFromList("bool_list", 10, out));
}

TEST_F(VariantDataTest, GetBooleanFromList_WrongType_TC024) {
    std::vector<int32_t> values = {1, 2, 3};
    ins->SetValue("int_list", values);
    
    bool out;
    EXPECT_FALSE(ins->GetBooleanFromList("int_list", 0, out));
}

// ============================================================================
// GetUInt64FromList Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetUInt64FromList_OutOfBounds_TC021) {
    std::vector<uint64_t> values = {100ULL, 200ULL};
    ins->SetValue("uint64_list", values);
    
    uint64_t out;
    EXPECT_FALSE(ins->GetUInt64FromList("uint64_list", 10, out));
}

TEST_F(VariantDataTest, GetUInt64FromList_WrongType_TC022) {
    std::vector<std::string> values = {"a", "b"};
    ins->SetValue("str_list", values);
    
    uint64_t out;
    EXPECT_FALSE(ins->GetUInt64FromList("str_list", 0, out));
}

// ============================================================================
// GetStringFromList Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetStringFromList_OutOfBounds_TC021) {
    std::vector<std::string> values = {"hello", "world"};
    ins->SetValue("str_list", values);
    
    std::string out;
    EXPECT_FALSE(ins->GetStringFromList("str_list", 10, out));
}

TEST_F(VariantDataTest, GetStringFromList_WrongType_TC022) {
    std::vector<int32_t> values = {1, 2, 3};
    ins->SetValue("int_list", values);
    
    std::string out;
    EXPECT_FALSE(ins->GetStringFromList("int_list", 0, out));
}

// ============================================================================
// GetDoubleFromList Additional Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetDoubleFromList_WrongType_TC021) {
    std::vector<std::string> values = {"a", "b"};
    ins->SetValue("str_list", values);
    
    double out;
    EXPECT_FALSE(ins->GetDoubleFromList("str_list", 0, out));
}

TEST_F(VariantDataTest, GetDoubleFromList_FloatVector_TC022) {
    std::vector<float> values = {1.5f, 2.5f, 3.5f};
    ins->SetValue("float_list", values);
    
    double out;
    EXPECT_TRUE(ins->GetDoubleFromList("float_list", 1, out));
    EXPECT_DOUBLE_EQ(2.5, out);
}

// ============================================================================
// GetInt64FromList Additional Branch Coverage Tests
// ============================================================================

TEST_F(VariantDataTest, GetInt64FromList_WrongType_TC021) {
    std::vector<std::string> values = {"a", "b"};
    ins->SetValue("str_list", values);
    
    int64_t out;
    EXPECT_FALSE(ins->GetInt64FromList("str_list", 0, out));
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
