#include "JsonSchemaValidator.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Logger.h"


std::string JsonSchemaValidator::schema_path_ = "";

void JsonSchemaValidator::init(const std::string& schemaPath) {
    schema_path_ = schemaPath;
}

JsonSchemaValidator& JsonSchemaValidator::getInstance() {
    if (schema_path_.empty()) {
        LOG_WARN << "Warning: JsonSchemaValidator::init() was not called. Using default path resolution.";
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
        LOG_ERROR << "Could not open schema file: " << path_to_use;
        return;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document sd;
    sd.ParseStream(isw);

    if (sd.HasParseError()) {
        LOG_ERROR << "Error parsing schema file: " << rapidjson::GetParseError_En(sd.GetParseError());
        return;
    }
    schema_doc_ = std::make_unique<rapidjson::SchemaDocument>(sd);
}

bool JsonSchemaValidator::validate(const rapidjson::Document& doc) {
    if (!schema_doc_) {
        LOG_ERROR << "Schema is not loaded. Cannot validate.";
        return false;
    }
    rapidjson::SchemaValidator validator(*schema_doc_);
    if (!doc.Accept(validator)) {
        // Use a PrettyWriter to format the error output
        rapidjson::StringBuffer sb;
        validator.GetInvalidSchemaPointer().Stringify(sb);
        LOG_ERROR << "Invalid schema: " << sb.GetString();
        LOG_ERROR << "Invalid keyword: " << validator.GetInvalidSchemaKeyword();
        sb.Clear();
        validator.GetInvalidDocumentPointer().Stringify(sb);
        LOG_ERROR << "Invalid document: " << sb.GetString();
        return false;
    }
    return true;
}

std::unique_ptr<rapidjson::Document> JsonSchemaValidator::validate(const std::string& json_str) {
    auto doc = std::make_unique<rapidjson::Document>();
    doc->Parse(json_str.c_str());
    if (doc->HasParseError()) {
        LOG_ERROR << "JSON parse error for validation: " << rapidjson::GetParseError_En(doc->GetParseError());
        return nullptr;
    }
/*
    if (!validate(*doc)) {
        return nullptr;
    }
*/
    return doc;
}
