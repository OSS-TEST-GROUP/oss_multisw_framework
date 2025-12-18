#include "gtest/gtest.h"
#include "StatusCodes.h"
#include "rapidjson/document.h"

// Helper function to verify the content of the error document
void VerifyErrorDoc(const rapidjson::Document& doc, const std::string& expected_number, const std::string& expected_reason, const std::string& expected_desc) {
    ASSERT_TRUE(doc.IsObject());
    ASSERT_TRUE(doc.HasMember("number"));
    ASSERT_TRUE(doc["number"].IsString());
    EXPECT_STREQ(doc["number"].GetString(), expected_number.c_str());

    ASSERT_TRUE(doc.HasMember("reason"));
    ASSERT_TRUE(doc["reason"].IsString());
    EXPECT_STREQ(doc["reason"].GetString(), expected_reason.c_str());

    ASSERT_TRUE(doc.HasMember("description"));
    ASSERT_TRUE(doc["description"].IsString());
    EXPECT_STREQ(doc["description"].GetString(), expected_desc.c_str());
}

TEST(StatusCodesTest, BadRequest) {
    auto doc = StatusCodes::BAD_REQUEST("Custom bad request message.");
    VerifyErrorDoc(doc, "400", "bad_request", "Custom bad request message.");
}

TEST(StatusCodesTest, BadRequestDefault) {
    auto doc = StatusCodes::BAD_REQUEST();
    VerifyErrorDoc(doc, "400", "bad_request", "The request is malformed");
}

TEST(StatusCodesTest, InvalidData) {
    auto doc = StatusCodes::INVALID_DATA("Custom invalid data message.");
    VerifyErrorDoc(doc, "400", "invalid_data", "Custom invalid data message.");
}

TEST(StatusCodesTest, InvalidDataDefault) {
    auto doc = StatusCodes::INVALID_DATA();
    VerifyErrorDoc(doc, "400", "invalid_data", "Data in the request is invalid");
}

TEST(StatusCodesTest, Unauthorized) {
    auto doc = StatusCodes::UNAUTHORIZED("Custom unauthorized message.");
    VerifyErrorDoc(doc, "401", "invalid_token", "Custom unauthorized message.");
}

TEST(StatusCodesTest, UnauthorizedDefault) {
    auto doc = StatusCodes::UNAUTHORIZED();
    VerifyErrorDoc(doc, "401", "invalid_token", "Access token is invalid");
}

TEST(StatusCodesTest, Forbidden) {
    auto doc = StatusCodes::FORBIDDEN("Custom forbidden message.");
    VerifyErrorDoc(doc, "403", "forbidden_request", "Custom forbidden message.");
}

TEST(StatusCodesTest, ForbiddenDefault) {
    auto doc = StatusCodes::FORBIDDEN();
    VerifyErrorDoc(doc, "403", "forbidden_request", "The server refuses to carry out the request");
}

TEST(StatusCodesTest, NotFound) {
    auto doc = StatusCodes::NOT_FOUND("Custom not found message.");
    VerifyErrorDoc(doc, "404", "unavailable_data", "Custom not found message.");
}

TEST(StatusCodesTest, NotFoundDefault) {
    auto doc = StatusCodes::NOT_FOUND();
    VerifyErrorDoc(doc, "404", "unavailable_data", "The requested data was not found");
}

TEST(StatusCodesTest, RequestTimeout) {
    auto doc = StatusCodes::REQUEST_TIMEOUT("Custom timeout message.");
    VerifyErrorDoc(doc, "408", "request_timeout", "Custom timeout message.");
}

TEST(StatusCodesTest, RequestTimeoutDefault) {
    auto doc = StatusCodes::REQUEST_TIMEOUT();
    VerifyErrorDoc(doc, "408", "request_timeout", "Subscribe duration limit exceeded");
}

TEST(StatusCodesTest, TooManyRequests) {
    auto doc = StatusCodes::TOO_MANY_REQUESTS("Custom rate limit message.");
    VerifyErrorDoc(doc, "429", "too_many_requests", "Custom rate limit message.");
}

TEST(StatusCodesTest, TooManyRequestsDefault) {
    auto doc = StatusCodes::TOO_MANY_REQUESTS();
    VerifyErrorDoc(doc, "429", "too_many_requests", "Rate-limiting due to too many requests");
}

TEST(StatusCodesTest, BadGateway) {
    auto doc = StatusCodes::BAD_GATEWAY("Custom bad gateway message.");
    VerifyErrorDoc(doc, "502", "bad_gateway", "Custom bad gateway message.");
}

TEST(StatusCodesTest, BadGatewayDefault) {
    auto doc = StatusCodes::BAD_GATEWAY();
    VerifyErrorDoc(doc, "502", "bad_gateway", "The upstream server response was invalid");
}

TEST(StatusCodesTest, ServiceUnavailable) {
    auto doc = StatusCodes::SERVICE_UNAVAILABLE("Custom service unavailable message.");
    VerifyErrorDoc(doc, "503", "service_unavailable", "Custom service unavailable message.");
}

TEST(StatusCodesTest, ServiceUnavailableDefault) {
    auto doc = StatusCodes::SERVICE_UNAVAILABLE();
    VerifyErrorDoc(doc, "503", "service_unavailable", "The server is temporarily unable to handle the request");
}

TEST(StatusCodesTest, GatewayTimeout) {
    auto doc = StatusCodes::GATEWAY_TIMEOUT("Custom gateway timeout message.");
    VerifyErrorDoc(doc, "504", "gateway_timeout", "Custom gateway timeout message.");
}

TEST(StatusCodesTest, GatewayTimeoutDefault) {
    auto doc = StatusCodes::GATEWAY_TIMEOUT();
    VerifyErrorDoc(doc, "504", "gateway_timeout", "The upstream server took too long to respond");
}
