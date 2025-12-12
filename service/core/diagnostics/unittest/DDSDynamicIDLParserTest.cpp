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

// Test Initialize with a valid directory that exists (successful path)
TEST_F(DDSDynamicIDLParserTest, Initialize_TC003_valid_directory_with_idl_files) {
    // The "idl" directory should exist in the test environment
    std::string valid_dir = "idl";
    bool result = parser->Initialize(valid_dir);
    EXPECT_TRUE(result);
}

// Test Initialize with empty string
TEST_F(DDSDynamicIDLParserTest, Initialize_TC004_empty_directory_string) {
    std::string empty_dir = "";
    // Empty string should fail since it doesn't exist as a directory
    EXPECT_FALSE(parser->Initialize(empty_dir));
}

// Test Initialize with a file path instead of directory
TEST_F(DDSDynamicIDLParserTest, Initialize_TC005_file_instead_of_directory) {
    // Try initializing with a file path - should fail is_directory check
    std::string file_path = "idl/DataTypeTestSimple.idl";
    EXPECT_FALSE(parser->Initialize(file_path));
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

// Test ParseAll without initialization (empty idl_directory_)
TEST_F(DDSDynamicIDLParserTest, ParseAll_TC002_no_initialization) {
    // Call ParseAll without initializing - should return empty vector
    auto type_support_list = parser->ParseAll();
    EXPECT_EQ(0, type_support_list.size());
}

// Test ParseAll with valid directory that has multiple IDL files
TEST_F(DDSDynamicIDLParserTest, ParseAll_TC003_iterate_all_idl_files) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_support_list = parser->ParseAll();
    // idl directory has DataTypeTestSimple.idl (1 struct), DataTypeTestComplex.idl (2 structs)
    // and possibly others
    EXPECT_GE(type_support_list.size(), 3);
    
    // Verify all parsed types have valid type_support
    for (auto& type : type_support_list) {
        EXPECT_TRUE(type != nullptr);
        EXPECT_TRUE(type->dyn_type != nullptr);
        EXPECT_TRUE(type->type_support != nullptr);
    }
}

TEST_F(DDSDynamicIDLParserTest, ExtractStructNamesFromIDL_TC001) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->ExtractStructNamesFromIDL("DataTypeTestComplex.idl");
    ASSERT_EQ(2, type_list.size());
    EXPECT_EQ("DataTypeTestComplex1", type_list[0]);
    EXPECT_EQ("DataTypeTestComplex2", type_list[1]);
}

// Test ExtractStructNamesFromIDL with non-existent file
TEST_F(DDSDynamicIDLParserTest, ExtractStructNamesFromIDL_TC002_nonexistent_file) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->ExtractStructNamesFromIDL("NonExistentFile.idl");
    // Should return empty list for non-existent file
    EXPECT_EQ(0, type_list.size());
}

// Test ExtractStructNamesFromIDL with simple IDL file (single struct)
TEST_F(DDSDynamicIDLParserTest, ExtractStructNamesFromIDL_TC003_single_struct) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->ExtractStructNamesFromIDL("DataTypeTestSimple.idl");
    ASSERT_EQ(1, type_list.size());
    EXPECT_EQ("DataTypeTestSimple", type_list[0]);
}

// Test Parse with valid IDL file that has multiple structs
TEST_F(DDSDynamicIDLParserTest, Parse_TC002_multiple_structs) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->Parse("DataTypeTestComplex.idl");
    ASSERT_EQ(2, type_list.size());
    
    // Verify each type has valid dyn_type and type_support
    for (auto& type : type_list) {
        EXPECT_TRUE(type->dyn_type != nullptr);
        EXPECT_TRUE(type->type_support != nullptr);
    }
}

// Test Parse with non-existent IDL file
TEST_F(DDSDynamicIDLParserTest, Parse_TC003_nonexistent_file) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->Parse("NonExistentFile.idl");
    // Should return empty list for non-existent file
    EXPECT_EQ(0, type_list.size());
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
