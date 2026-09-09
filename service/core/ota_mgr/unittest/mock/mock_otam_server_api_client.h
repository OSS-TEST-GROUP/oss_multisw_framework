#ifndef MOCK_OTAM_SERVER_API_CLIENT_H_
#define MOCK_OTAM_SERVER_API_CLIENT_H_

// QA(docs/qa): OtamServerApiClient(추상 HTTP 인터페이스)의 gmock.
// server_service 의 GetOtaSettings/SetOtaSettings/RunOtaUpdate 로직을 실 ArgoCD
// HTTP 호출 없이 검증하기 위한 mock. server_service.h 의 InjectApiClientForTest 로 주입.

#include "otam_server_service/otam_server_api_client.h"

#include <gmock/gmock.h>
#include <string>

class MockOtamServerApiClient : public OtamServerApiClient {
 public:
  MOCK_METHOD(std::string, Post, (const std::string& endpoint, const std::string& body), (override));
  MOCK_METHOD(std::string, Get, (const std::string& endpoint, const std::string& body), (override));
  MOCK_METHOD(std::string, Put, (const std::string& endpoint, const std::string& body), (override));
  MOCK_METHOD(std::string, Delete, (const std::string& endpoint, const std::string& body), (override));
  MOCK_METHOD(std::string, Patch, (const std::string& endpoint, const std::string& body), (override));
  MOCK_METHOD(void, SetAuthToken, (const std::string& token), (override));
  MOCK_METHOD(void, AddHeader, (const std::string& key, const std::string& value), (override));
  MOCK_METHOD(void, RemoveHeader, (const std::string& key), (override));
};

#endif  // MOCK_OTAM_SERVER_API_CLIENT_H_
