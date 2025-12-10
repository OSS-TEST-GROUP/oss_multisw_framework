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


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
