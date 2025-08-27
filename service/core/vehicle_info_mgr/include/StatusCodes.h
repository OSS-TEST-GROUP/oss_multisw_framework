#ifndef STATUS_CODES_H
#define STATUS_CODES_H

#include <string>
#include <map>

#include "rapidjson/document.h"

struct ErrorStatus {
    std::string number;
    std::string reason;
    std::string description;

    std::map<std::string, std::string> toMap() const {
        return {
            {"number", number},
            {"reason", reason},
            {"description", description}
        };
    }
};

class StatusCodes {
public:
    // Client Errors (4xx)
    static rapidjson::Document BAD_REQUEST(const std::string& desc = "The request is malformed");
    static rapidjson::Document INVALID_DATA(const std::string& desc = "Data in the request is invalid");
    static rapidjson::Document UNAUTHORIZED(const std::string& desc = "Access token is invalid");
    static rapidjson::Document FORBIDDEN(const std::string& desc = "The server refuses to carry out the request");
    static rapidjson::Document NOT_FOUND(const std::string& desc = "The requested data was not found");
    static rapidjson::Document REQUEST_TIMEOUT(const std::string& desc = "Subscribe duration limit exceeded");
    static rapidjson::Document TOO_MANY_REQUESTS(const std::string& desc = "Rate-limiting due to too many requests");

    // Server Errors (5xx)
    static rapidjson::Document BAD_GATEWAY(const std::string& desc = "The upstream server response was invalid");
    static rapidjson::Document SERVICE_UNAVAILABLE(const std::string& desc = "The server is temporarily unable to handle the request");
    static rapidjson::Document GATEWAY_TIMEOUT(const std::string& desc = "The upstream server took too long to respond");
};

#endif // STATUS_CODES_H
