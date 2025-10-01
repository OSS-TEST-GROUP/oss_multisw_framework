#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "transmitter/dds/DDSDynamicIDLParser.cpp"
#include "unittest/mock/MockLogger.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;

using namespace diagnostic_manager;
class DDSDynamicIDLParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = new DDSDynamicIDLParser();
        Mock_Logger = new MockLogger();
    }

    void TearDown() override {
        delete parser;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    DDSDynamicIDLParser* parser;
};

TEST_F(DDSDynamicIDLParserTest, Initialize_TC001) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));
}

TEST_F(DDSDynamicIDLParserTest, Initialize_TC002_no_directory) {
    std::string valid_dir = "idl2";
    EXPECT_FALSE(parser->Initialize(valid_dir));
}

TEST_F(DDSDynamicIDLParserTest, Parse_TC001) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->Parse("DataTypeTestSimple.idl");
    ASSERT_TRUE(type_list.size() == 1);
    EXPECT_TRUE(type_list[0]->dyn_type != nullptr);
    EXPECT_EQ("DataTypeTestSimple", type_list[0]->type_support->get_type_name());
}

TEST_F(DDSDynamicIDLParserTest, ParseAll_TC001) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_support_list = parser->ParseAll();
    EXPECT_EQ(3, type_support_list.size());
}

TEST_F(DDSDynamicIDLParserTest, ExtractStructNamesFromIDL_TC001) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->ExtractStructNamesFromIDL("DataTypeTestComplex.idl");
    ASSERT_EQ(2, type_list.size());
    EXPECT_EQ("DataTypeTestComplex1", type_list[0]);
    EXPECT_EQ("DataTypeTestComplex2", type_list[1]);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
