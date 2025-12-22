#include "gtest/gtest.h"
#include "JsonSchemaValidator.h"
#include <string>
#include <fstream>
#include <streambuf>

class JsonSchemaValidatorTest : public ::testing::Test {
protected:
    std::optional<std::string> getEnv(const std::string& key) {
        const char* val = std::getenv(key.c_str());
        return val ? std::optional<std::string>(val) : std::nullopt;
    }

    void SetUp() override {
        // Create a dummy schema for testing purposes
        // std::ofstream schema_file("build/test_schema.json");
        // schema_file << R"({
        //     "type": "object",
        //     "properties": {
        //         "name": {"type": "string"},
        //         "age": {"type": "integer"}
        //     },
        //     "required": ["name", "age"]
        // })";
        // schema_file.close();

        auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
        test_path.append("/testfiles/test-schema.json");

        JsonSchemaValidator::init(test_path);
    }

    void TearDown() override {
        //std::remove("build/test-schema.json");
    }
};

TEST_F(JsonSchemaValidatorTest, ValidJson) {
    std::string valid_json_str = R"({"name": "John Doe", "age": 30})";
    rapidjson::Document doc;
    doc.Parse(valid_json_str.c_str());
    ASSERT_FALSE(doc.HasParseError());
    EXPECT_TRUE(JsonSchemaValidator::getInstance().validate(doc));
}

TEST_F(JsonSchemaValidatorTest, InvalidJson) {
    std::string invalid_json_str = R"({"name": "John Doe", "age": "thirty"})";
    rapidjson::Document doc;
    doc.Parse(invalid_json_str.c_str());
    ASSERT_FALSE(doc.HasParseError());
    EXPECT_FALSE(JsonSchemaValidator::getInstance().validate(doc));
}

TEST_F(JsonSchemaValidatorTest, MalformedJson) {
    std::string malformed_json = R"({"name": "John Doe", "age": 30,)";
    auto doc = JsonSchemaValidator::getInstance().validate(malformed_json);
    ASSERT_EQ(doc, nullptr);
}

TEST_F(JsonSchemaValidatorTest, MissingRequiredField) {
    std::string missing_field_json_str = R"({"name": "John Doe"})";
    rapidjson::Document doc;
    doc.Parse(missing_field_json_str.c_str());
    ASSERT_FALSE(doc.HasParseError());
    EXPECT_FALSE(JsonSchemaValidator::getInstance().validate(doc));
}
