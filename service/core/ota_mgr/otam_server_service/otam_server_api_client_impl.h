#pragma once
#include "otam_common/api_client.h"
#include <string>
#include <map>

class OtamServerApiClientImpl : public ApiClient {
public:
    explicit OtamServerApiClientImpl(
        const std::string& base_url,
        const std::map<std::string, std::string>& default_headers = {})
        : base_url_(base_url),
          default_headers_(default_headers) {}

    std::string Get(
        const std::string& endpoint,
        const std::string& body) override;

    std::string Post(
        const std::string& endpoint,
        const std::string& body) override;

    std::string Put(
        const std::string& endpoint,
        const std::string& body) override;

    std::string Delete(
        const std::string& endpoint,
        const std::string& body) override;

    std::string Patch(
        const std::string& endpoint,
        const std::string& body) override;

    void SetAuthToken(const std::string& token);
    void AddHeader(const std::string& key,
                   const std::string& value);
    void RemoveHeader(const std::string& key);

private:
    std::string SendRequest(const std::string& method,
                            const std::string& endpoint,
                            const std::string& body);

    std::string base_url_;
    std::map<std::string, std::string> default_headers_;
};