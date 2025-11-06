#include "JsonSchemaValidator.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/en.h"

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>
#include <valijson/validation_results.hpp>


std::string JsonSchemaValidator::schema_path_ = "";

void JsonSchemaValidator::init(const std::string& schemaPath) {
    schema_path_ = schemaPath;
}

JsonSchemaValidator& JsonSchemaValidator::getInstance() {
    if (schema_path_.empty()) {
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
    rapidjson::Document schemaDoc;
    schemaDoc.ParseStream(isw);

    if (schemaDoc.HasParseError()) {
        std::cerr << "Error parsing schema file: " << rapidjson::GetParseError_En(schemaDoc.GetParseError()) << std::endl;
        return;
    }

    valijson::SchemaParser parser;
    valijson::adapters::RapidJsonAdapter schemaAdapter(schemaDoc);
    try {
        parser.populateSchema(schemaAdapter, schema_);
    } catch (const std::exception &e) {
        std::cerr << "Failed to populate schema: " << e.what() << std::endl;
    }
}

bool JsonSchemaValidator::validate(const rapidjson::Document& doc) {
    //valijson::Validator validator;
    valijson::Validator validator(valijson::Validator::kWeakTypes);
    valijson::adapters::RapidJsonAdapter docAdapter(doc);
    valijson::ValidationResults results;

    if (!validator.validate(schema_, docAdapter, &results)) {
        std::cerr << "----------------------------------------" << std::endl;
        std::cerr << "Schema validation failed. Detailed report:" << std::endl;
        valijson::ValidationResults::Error error;
        unsigned int errorNum = 1;
        while (results.popError(error)) {
            std::string context;
            for (const auto &path_part : error.context) {
                context += path_part;
            }
            std::cerr << "Error #" << errorNum++ << ":" << std::endl;
            std::cerr << "  context: " << context << std::endl;
            std::cerr << "  desc:    " << error.description << std::endl;
        }
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

    if (!validate(*doc)) {
        return nullptr;
    }

    return doc;
}
