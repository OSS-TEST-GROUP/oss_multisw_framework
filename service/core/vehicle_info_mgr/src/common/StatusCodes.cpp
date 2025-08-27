#include "StatusCodes.h"

static rapidjson::Document createErrorDoc(const std::string& number, const std::string& reason, const std::string& description) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    doc.AddMember("number", rapidjson::Value(number.c_str(), allocator), allocator);
    doc.AddMember("reason", rapidjson::Value(reason.c_str(), allocator), allocator);
    doc.AddMember("description", rapidjson::Value(description.c_str(), allocator), allocator);
    return doc;
}

// Client Errors (4xx)
rapidjson::Document StatusCodes::BAD_REQUEST(const std::string& desc) {
    return createErrorDoc("400", "bad_request", desc);
}
rapidjson::Document StatusCodes::INVALID_DATA(const std::string& desc) {
    return createErrorDoc("400", "invalid_data", desc);
}
rapidjson::Document StatusCodes::UNAUTHORIZED(const std::string& desc) {
    return createErrorDoc("401", "invalid_token", desc);
}
rapidjson::Document StatusCodes::FORBIDDEN(const std::string& desc) {
    return createErrorDoc("403", "forbidden_request", desc);
}
rapidjson::Document StatusCodes::NOT_FOUND(const std::string& desc) {
    return createErrorDoc("404", "unavailable_data", desc);
}
rapidjson::Document StatusCodes::REQUEST_TIMEOUT(const std::string& desc) {
    return createErrorDoc("408", "request_timeout", desc);
}
rapidjson::Document StatusCodes::TOO_MANY_REQUESTS(const std::string& desc) {
    return createErrorDoc("429", "too_many_requests", desc);
}

// Server Errors (5xx)
rapidjson::Document StatusCodes::BAD_GATEWAY(const std::string& desc) {
    return createErrorDoc("502", "bad_gateway", desc);
}
rapidjson::Document StatusCodes::SERVICE_UNAVAILABLE(const std::string& desc) {
    return createErrorDoc("503", "service_unavailable", desc);
}
rapidjson::Document StatusCodes::GATEWAY_TIMEOUT(const std::string& desc) {
    return createErrorDoc("504", "gateway_timeout", desc);
}
