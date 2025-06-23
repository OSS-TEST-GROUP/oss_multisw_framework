#include <gtest/gtest.h>
#include "service/ArgumentConfig.hpp"
#include "mock/mock_argumentconfig.hpp"

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

using namespace lifecycle_manager;


class ArgumentConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No setup required for now
    }
    void TearDown() override {
        // No teardown required for now
    }
};

TEST_F(ArgumentConfigTest, ConstructorAndGetters) {
    std::string name = "arg1";
    std::string description = "This is a test argument";
    std::string type = "string";
    std::string default_value = "default";

    ArgumentConfig arg_config(name, description, type, default_value);

    EXPECT_EQ(arg_config.GetName(), name);
    EXPECT_EQ(arg_config.GetDescription(), description);
    EXPECT_EQ(arg_config.GetType(), type);
    EXPECT_EQ(arg_config.GetArgument(), default_value);
}

TEST_F(ArgumentConfigTest, EmptyStrings) {
    ArgumentConfig arg_config("", "", "", "");

    EXPECT_EQ(arg_config.GetName(), "");
    EXPECT_EQ(arg_config.GetDescription(), "");
    EXPECT_EQ(arg_config.GetType(), "");
    EXPECT_EQ(arg_config.GetArgument(), "");
}

TEST_F(ArgumentConfigTest, DifferentTypes) {
    ArgumentConfig arg_config("arg2", "desc", "int", "42");

    EXPECT_EQ(arg_config.GetName(), "arg2");
    EXPECT_EQ(arg_config.GetDescription(), "desc");
    EXPECT_EQ(arg_config.GetType(), "int");
    EXPECT_EQ(arg_config.GetArgument(), "42");
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
