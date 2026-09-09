#include "gtest/gtest.h"
#define private public
#include "JsonSchemaValidator.h"
#undef private
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

TEST_F(JsonSchemaValidatorTest, ValidateValidJsonString) {
    std::string valid_json = R"({"name": "Alice", "age": 25})";
    auto doc = JsonSchemaValidator::getInstance().validate(valid_json);
    ASSERT_NE(doc, nullptr);
    ASSERT_FALSE(doc->HasParseError());
    EXPECT_STREQ((*doc)["name"].GetString(), "Alice");
}

TEST_F(JsonSchemaValidatorTest, ConstructorEdgeCasesAndUnloadedSchema) {
    // 1. Non-existent schema file
    {
        JsonSchemaValidator invalid_val("/non_existent_path/schema.json");
        rapidjson::Document doc;
        doc.SetObject();
        EXPECT_FALSE(invalid_val.validate(doc));
    }

    // 2. Corrupted JSON schema file
    {
        std::string bad_schema_path = "/tmp/bad_schema.json";
        std::ofstream ofs(bad_schema_path);
        ofs << "{ invalid json schema";
        ofs.close();

        JsonSchemaValidator bad_val(bad_schema_path);
        rapidjson::Document doc;
        doc.SetObject();
        EXPECT_FALSE(bad_val.validate(doc));
        std::remove(bad_schema_path.c_str());
    }

    // 3. Empty schema path fallback
    {
        JsonSchemaValidator default_val("");
    }
}
