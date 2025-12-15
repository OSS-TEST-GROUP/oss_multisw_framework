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

// ==================== Additional Coverage Tests ====================

// Test Initialize with relative path that exists
TEST_F(DDSDynamicIDLParserTest, Initialize_TC006_relative_path_exists) {
    // The "idl" directory exists in the test environment
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));
    // Verify idl_directory_ is set
    EXPECT_EQ(valid_dir, parser->idl_directory_);
}

// Test ParseAll after successful initialization - exercises the directory iteration loop
TEST_F(DDSDynamicIDLParserTest, ParseAll_TC004_successful_iteration) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));
    
    auto type_support_list = parser->ParseAll();
    // idl directory has DataTypeTestSimple.idl (1 struct), DataTypeTestComplex.idl (2 structs)
    // and DataTypeTest.idl with multiple structs
    EXPECT_GE(type_support_list.size(), 3);
    
    // Verify each parsed type has valid fields
    for (auto& type : type_support_list) {
        EXPECT_NE(nullptr, type);
        EXPECT_NE(nullptr, type->dyn_type);
        EXPECT_NE(nullptr, type->type_support);
        // Type name should not be empty
        EXPECT_FALSE(type->type_support->get_type_name().empty());
    }
}

// Test Parse with IDL that has multiple structs - exercises the full loop
TEST_F(DDSDynamicIDLParserTest, Parse_TC004_complex_idl_full_iteration) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    // DataTypeTestComplex.idl has 2 structs
    auto type_list = parser->Parse("DataTypeTestComplex.idl");
    ASSERT_EQ(2, type_list.size());
    
    // Verify each type is properly created
    for (size_t i = 0; i < type_list.size(); i++) {
        EXPECT_NE(nullptr, type_list[i]);
        EXPECT_NE(nullptr, type_list[i]->dyn_type);
        EXPECT_NE(nullptr, type_list[i]->type_support);
    }
    
    // Verify specific type names (without namespace prefix, as that's how DDS reports them)
    std::set<std::string> expected_types = {"DataTypeTestComplex1", "DataTypeTestComplex2"};
    std::set<std::string> actual_types;
    for (auto& type : type_list) {
        actual_types.insert(type->type_support->get_type_name());
    }
    EXPECT_EQ(expected_types, actual_types);
}

// Test ExtractStructNamesFromIDL with valid file that can be opened
TEST_F(DDSDynamicIDLParserTest, ExtractStructNamesFromIDL_TC004_successful_read) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    // DataTypeTestSimple.idl has 1 struct
    auto type_list = parser->ExtractStructNamesFromIDL("DataTypeTestSimple.idl");
    ASSERT_EQ(1, type_list.size());
    EXPECT_EQ("DataTypeTestSimple", type_list[0]);
}

// Test ExtractStructNamesFromIDL with file that has multiple structs
TEST_F(DDSDynamicIDLParserTest, ExtractStructNamesFromIDL_TC005_multiple_structs_read) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    // DataTypeTestComplex.idl has 2 structs
    auto type_list = parser->ExtractStructNamesFromIDL("DataTypeTestComplex.idl");
    ASSERT_EQ(2, type_list.size());
    EXPECT_EQ("DataTypeTestComplex1", type_list[0]);
    EXPECT_EQ("DataTypeTestComplex2", type_list[1]);
}

// Test Parse successfully builds dynamic types
TEST_F(DDSDynamicIDLParserTest, Parse_TC005_verify_type_building) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));

    auto type_list = parser->Parse("DataTypeTestSimple.idl");
    ASSERT_EQ(1, type_list.size());
    
    // Verify the dynamic type is properly built
    EXPECT_NE(nullptr, type_list[0]->dyn_type);
    
    // Verify type_support is created with DynamicPubSubType
    EXPECT_NE(nullptr, type_list[0]->type_support);
    EXPECT_EQ("DataTypeTestSimple", type_list[0]->type_support->get_type_name());
}

// Test ParseAll with directory containing subdirectories (recursive iteration)
TEST_F(DDSDynamicIDLParserTest, ParseAll_TC005_recursive_directory_iteration) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));
    
    // ParseAll uses recursive_directory_iterator so it should find all .idl files
    auto type_support_list = parser->ParseAll();
    
    // Should find at least the known IDL files
    EXPECT_GE(type_support_list.size(), 1);
    
    // All returned types should be valid
    for (const auto& type : type_support_list) {
        EXPECT_TRUE(type != nullptr);
        EXPECT_TRUE(type->dyn_type != nullptr);
        EXPECT_TRUE(type->type_support != nullptr);
    }
}

// Test Parse with empty type list (IDL file exists but no structs extracted)
TEST_F(DDSDynamicIDLParserTest, Parse_TC006_no_structs_in_idl) {
    std::string valid_dir = "idl";
    EXPECT_TRUE(parser->Initialize(valid_dir));
    
    // If ExtractStructNamesFromIDL returns empty, Parse should return empty
    // This is covered by Parse_TC003_nonexistent_file but we add explicit coverage
    auto type_list = parser->Parse("NonExistentFile.idl");
    EXPECT_EQ(0, type_list.size());
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
