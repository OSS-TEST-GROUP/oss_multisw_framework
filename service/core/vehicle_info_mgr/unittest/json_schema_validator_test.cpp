#include "gtest/gtest.h"
#include "JsonSchemaValidator.h"
#include <string>
#include <fstream>
#include <streambuf>

class JsonSchemaValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a dummy schema for testing purposes
        std::ofstream schema_file("build/test_schema.json");
        schema_file << R"({
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "age": {"type": "integer"}
            },
            "required": ["name", "age"]
        })";
        schema_file.close();

        JsonSchemaValidator::init("build/test_schema.json");
    }

    void TearDown() override {
        std::remove("build/test_schema.json");
    }
};

TEST_F(JsonSchemaValidatorTest, ValidJson) {
    std::string valid_json = R"({"name": "John Doe", "age": 30})";
    auto doc = JsonSchemaValidator::getInstance().validate(valid_json);
    ASSERT_NE(doc, nullptr);
}

TEST_F(JsonSchemaValidatorTest, InvalidJson) {
    std::string invalid_json = R"({"name": "John Doe", "age": "thirty"})";
    auto doc = JsonSchemaValidator::getInstance().validate(invalid_json);
    ASSERT_NE(doc, nullptr);
}

TEST_F(JsonSchemaValidatorTest, MalformedJson) {
    std::string malformed_json = R"({"name": "John Doe", "age": 30,)";
    auto doc = JsonSchemaValidator::getInstance().validate(malformed_json);
    ASSERT_EQ(doc, nullptr);
}

TEST_F(JsonSchemaValidatorTest, MissingRequiredField) {
    std::string missing_field_json = R"({"name": "John Doe"})";
    auto doc = JsonSchemaValidator::getInstance().validate(missing_field_json);
    ASSERT_NE(doc, nullptr);
}
