#ifndef JSON_SCHEMA_VALIDATOR_H
#define JSON_SCHEMA_VALIDATOR_H

//#define RAPIDJSON_HAS_STDSTRING 1   

#include "rapidjson/document.h"
#include <string>
#include <memory>
#include "valijson/schema.hpp"

class JsonSchemaValidator {
public:
    static void init(const std::string& schemaPath);
    static JsonSchemaValidator& getInstance();
    
    // Validates a string, and if successful, returns a unique_ptr to the parsed Document.
    // Returns nullptr on parse or validation failure.
    std::unique_ptr<rapidjson::Document> validate(const std::string& json_str);

private:
    bool validate(const rapidjson::Document& doc); // Keep the old one for internal use
    JsonSchemaValidator(const std::string& schemaPath); // Private constructor now takes path
    JsonSchemaValidator(const JsonSchemaValidator&) = delete;
    JsonSchemaValidator& operator=(const JsonSchemaValidator&) = delete;

    valijson::Schema schema_;
    static std::string schema_path_;
};

#endif // JSON_SCHEMA_VALIDATOR_H
