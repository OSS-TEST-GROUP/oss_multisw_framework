#ifndef OTAM_SERVER_API_CLIENT_IMPL_H_
#define OTAM_SERVER_API_CLIENT_IMPL_H_
#include "otam_server_api_client.h"
#include <string>
#include <map>

class OtamServerApiClientImpl : public OtamServerApiClient {
public:
    explicit OtamServerApiClientImpl(
        const std::string& base_url,
        const std::map<std::string, std::string>& default_headers = {})
        : base_url_(base_url),
          default_headers_(default_headers) {}

    OtamServerApiClientImpl() = default;
    ~OtamServerApiClientImpl() override = default;

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

    void SetAuthToken(const std::string& token) override;
    void AddHeader(const std::string& key,
                   const std::string& value) override;
    void RemoveHeader(const std::string& key) override;

private:
    std::string SendRequest(const std::string& method,
                            const std::string& endpoint,
                            const std::string& body);

    std::string base_url_;
    std::map<std::string, std::string> default_headers_;
};

#endif //OTAM_SERVER_API_CLIENT_IMPL_H_