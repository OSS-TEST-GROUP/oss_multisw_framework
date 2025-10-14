#include "JsonSchemaValidator.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/en.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

std::string JsonSchemaValidator::schema_path_ = "";

void JsonSchemaValidator::init(const std::string& schemaPath) {
    schema_path_ = schemaPath;
}

JsonSchemaValidator& JsonSchemaValidator::getInstance() {
    if (schema_path_.empty()) {
        // Fallback or default behavior if init() was not called.
        // This could throw an exception or use a hardcoded default path.
        // For now, we'll proceed, and the constructor will likely fail.
        std::cerr << "Warning: JsonSchemaValidator::init() was not called. Using default path resolution." << std::endl;
    }
    static JsonSchemaValidator instance(schema_path_);
    return instance;
}

JsonSchemaValidator::JsonSchemaValidator(const std::string& schemaPath) {
    std::string path_to_use = schemaPath;
    if (path_to_use.empty()) {
        path_to_use = "config/vissv3.0-schema.json";
    }

    std::ifstream ifs(path_to_use);
    if (!ifs) {
        std::cerr << "Could not open schema file: " << path_to_use << std::endl;
        return;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document sd;
    sd.ParseStream(isw);

    if (sd.HasParseError()) {
        std::cerr << "Error parsing schema file: " << rapidjson::GetParseError_En(sd.GetParseError()) << std::endl;
        return;
    }
    
    schema_ = std::make_unique<rapidjson::SchemaDocument>(sd);
}

bool JsonSchemaValidator::validate(const rapidjson::Document& doc) {
    if (!schema_) {
        std::cerr << "Schema is not loaded." << std::endl;
        return false;
    }

    rapidjson::SchemaValidator validator(*schema_);
    if (!doc.Accept(validator)) {
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
        validator.GetError().Accept(writer);
        
        std::cerr << "----------------------------------------" << std::endl;
        std::cerr << "Schema validation failed. Detailed report:" << std::endl;
        std::cerr << sb.GetString() << std::endl;
        std::cerr << "----------------------------------------" << std::endl;
        return false;
    }
    return true;
}

std::unique_ptr<rapidjson::Document> JsonSchemaValidator::validate(const std::string& json_str) {
    auto doc = std::make_unique<rapidjson::Document>();
    doc->Parse(json_str.c_str());
    if (doc->HasParseError()) {
        std::cerr << "JSON parse error for validation: " << rapidjson::GetParseError_En(doc->GetParseError()) << std::endl;
        return nullptr;
    }

    if (!validate(*doc)) { // Call the private validate method
        return doc;//nullptr;
    }

    return doc;
}
