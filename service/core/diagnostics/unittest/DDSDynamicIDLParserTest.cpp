#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "transmitter/dds/DDSDynamicIDLParser.cpp"

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
    }

    void TearDown() override {
        delete parser;
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

    auto type_support = parser->Parse("DataTypeTestSimple.idl");
    EXPECT_TRUE(type_support != nullptr);
    EXPECT_TRUE(type_support->dyn_type != nullptr);
    EXPECT_EQ("DataTypeTestSimple", type_support->type_support->get_type_name());
}

TEST_F(DDSDynamicIDLParserTest, ParseAll_TC001) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_support_list = parser->ParseAll();
    EXPECT_TRUE(type_support_list.size() == 1);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}