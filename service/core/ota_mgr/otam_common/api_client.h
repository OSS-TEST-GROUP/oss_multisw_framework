#pragma once
#include <string>

class ApiClient {
    virtual ~ApiClient() = default;
    virtual std::string Post(const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Get (const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Put (const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Delete(const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Patch(const std::string& endpoint, const std::string& body) = 0;
};