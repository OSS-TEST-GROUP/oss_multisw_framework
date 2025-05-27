#include <gtest/gtest.h>

#include "diagnostic/VariantData.cpp"

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class VariantDataTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        ins = new VariantData();
    }
    virtual void TearDown(){
        delete ins;
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


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
