#include <gtest/gtest.h>

#include "service/ArgumentConfig.hpp"

using namespace lifecycle_manager;


class ArgumentConfigTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new ArgumentConfig("test_arg", "This is a test argument", "string", "default_value");
    }
    virtual void TearDown(){
        delete ins;
    }

    ArgumentConfig* ins;
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

TEST_F(ArgumentConfigTest, SpecialCharactersInStrings) {
    std::string name_with_special = "arg_name-123";
    std::string desc_with_special = "Description with spaces and symbols: !@#$%";
    std::string type_with_special = "custom_type";
    std::string default_with_special = "/path/to/file.txt";

    ArgumentConfig arg_config(name_with_special, desc_with_special, type_with_special, default_with_special);

    EXPECT_EQ(arg_config.GetName(), name_with_special);
    EXPECT_EQ(arg_config.GetDescription(), desc_with_special);
    EXPECT_EQ(arg_config.GetType(), type_with_special);
    EXPECT_EQ(arg_config.GetArgument(), default_with_special);
}

TEST_F(ArgumentConfigTest, LongStrings) {
    std::string long_name(500, 'a');
    std::string long_desc(1000, 'b');
    std::string long_type(100, 'c');
    std::string long_default(200, 'd');

    ArgumentConfig arg_config(long_name, long_desc, long_type, long_default);

    EXPECT_EQ(arg_config.GetName(), long_name);
    EXPECT_EQ(arg_config.GetDescription(), long_desc);
    EXPECT_EQ(arg_config.GetType(), long_type);
    EXPECT_EQ(arg_config.GetArgument(), long_default);
}

TEST_F(ArgumentConfigTest, CopyConstructor) {
    ArgumentConfig original("name1", "desc1", "type1", "default1");
    ArgumentConfig copy(original);

    EXPECT_EQ(copy.GetName(), original.GetName());
    EXPECT_EQ(copy.GetDescription(), original.GetDescription());
    EXPECT_EQ(copy.GetType(), original.GetType());
    EXPECT_EQ(copy.GetArgument(), original.GetArgument());
}

TEST_F(ArgumentConfigTest, BooleanTypeArgument) {
    ArgumentConfig arg_true("bool_arg", "boolean argument", "bool", "true");
    ArgumentConfig arg_false("bool_arg2", "boolean argument 2", "bool", "false");

    EXPECT_EQ(arg_true.GetArgument(), "true");
    EXPECT_EQ(arg_false.GetArgument(), "false");
}

TEST_F(ArgumentConfigTest, FloatTypeArgument) {
    ArgumentConfig arg_float("float_arg", "floating point", "float", "3.14159");

    EXPECT_EQ(arg_float.GetType(), "float");
    EXPECT_EQ(arg_float.GetArgument(), "3.14159");
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
