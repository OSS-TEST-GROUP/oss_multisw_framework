#ifndef OTAM_SERVER_API_CLIENT_H_
#define OTAM_SERVER_API_CLIENT_H_
#include <string>

class OtamServerApiClient {
public:
    virtual ~OtamServerApiClient() = default;
    virtual std::string Post(const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Get (const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Put (const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Delete(const std::string& endpoint, const std::string& body) = 0;
    virtual std::string Patch(const std::string& endpoint, const std::string& body) = 0;
    virtual void SetAuthToken(const std::string& token) = 0;
    virtual void AddHeader(const std::string& key, const std::string& value) = 0;
    virtual void RemoveHeader(const std::string& key) = 0;
};

#endif //OTAM_SERVER_API_CLIENT_H_