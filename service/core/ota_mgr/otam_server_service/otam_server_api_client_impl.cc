#include "otam_server_api_client_impl.hpp"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

namespace {
size_t WriteCallback(void* contents,
                     size_t size,
                     size_t nmemb,
                     std::string* output) {
    const size_t total = size * nmemb;
    output->append(static_cast<char*>(contents), total);
    return total;
}
}  // namespace

void OtamServerApiClientImpl::SetAuthToken(const std::string& token) {
    default_headers_["Authorization"] = "Bearer " + token;
}

void OtamServerApiClientImpl::AddHeader(
    const std::string& key,
    const std::string& value) {
    default_headers_[key] = value;
}

void OtamServerApiClientImpl::RemoveHeader(const std::string& key) {
    default_headers_.erase(key);
}

std::string OtamServerApiClientImpl::SendRequest(
    const std::string& method,
    const std::string& endpoint,
    const std::string& body) {
    std::string response;
    std::string url = base_url_ + endpoint;

    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("Failed to initialize CURL");

    struct curl_slist* headers = nullptr;

    // Always include Authorization header
    auto it = default_headers_.find("Authorization");
    if (it != default_headers_.end()) {
        std::string auth = it->first + ": " + it->second;
        headers = curl_slist_append(headers, auth.c_str());
    }

    // Add Content-Type header only when explicitly added
    if (default_headers_.count("Content-Type") > 0) {
        std::string ct = "Content-Type: " +
                         default_headers_.at("Content-Type");
        headers = curl_slist_append(headers, ct.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (!body.empty())
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::ostringstream oss;
        oss << "curl_easy_perform() failed: "
            << curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::runtime_error(oss.str());
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code >= 300) {
        std::cerr << "[OtamServerApiClientImpl] HTTP "
                  << http_code << " from " << url << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

std::string OtamServerApiClientImpl::Get(
    const std::string& ep,
    const std::string& body) {
    return SendRequest("GET", ep, body);
}

std::string OtamServerApiClientImpl::Post(
    const std::string& ep,
    const std::string& body) {
    return SendRequest("POST", ep, body);
}

std::string OtamServerApiClientImpl::Put(
    const std::string& ep,
    const std::string& body) {
    return SendRequest("PUT", ep, body);
}

std::string OtamServerApiClientImpl::Delete(
    const std::string& ep,
    const std::string& body) {
    return SendRequest("DELETE", ep, body);
}

std::string OtamServerApiClientImpl::Patch(
    const std::string& ep,
    const std::string& body) {
    return SendRequest("PATCH", ep, body);
}