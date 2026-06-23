#ifndef REST_HTTP_CLIENT_H
#define REST_HTTP_CLIENT_H

#include <string>
#include <curl/curl.h>
#include <iostream>
#include "Logger.h"

class RestHttpClient {
public:
    RestHttpClient() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        m_curl = curl_easy_init();
    }

    ~RestHttpClient() {
        if (m_curl) {
            curl_easy_cleanup(m_curl);
        }
        curl_global_cleanup();
    }

    bool postBinary(const std::string& url, const void* data, size_t size, const std::string& accessToken = "") {
        if (!m_curl) return false;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        if (!accessToken.empty()) {
            std::string authHeader = "access_token: " + accessToken;
            headers = curl_slist_append(headers, authHeader.c_str());
        }

        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, (long)size);
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(m_curl);
        bool success = (res == CURLE_OK);

        if (!success) {
            LOG_ERROR << "CURL POST failed: " << curl_easy_strerror(res);
        } else {
            long response_code;
            curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
            LOG_INFO << "CURL POST success. Response Code: " << response_code;
            success = (response_code >= 200 && response_code < 300);
        }

        curl_slist_free_all(headers);
        return success;
    }

private:
    CURL* m_curl;
};

#endif // REST_HTTP_CLIENT_H
