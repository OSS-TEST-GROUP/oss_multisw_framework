#ifndef DIAGNOSTICS_UNITTEST_MOCK_REQUEST_BASE_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_REQUEST_BASE_MOCK_HPP_

#include <gmock/gmock.h>
#include "executor/RequestBase.hpp"

namespace diagnostic_manager {

class RequestBaseMock {
    MOCK_METHOD0(GetRequestId, uint64_t());
    MOCK_METHOD0(GetRequestedTime, std::chrono::steady_clock::time_point());
};

RequestBaseMock* Mock_RequestBaseMock;

RequestBase::RequestBase() : request_id_(1U) {}
RequestBase::~RequestBase() {}

uint64_t RequestBase::GetRequestId() {
    return Mock_RequestBaseMock->GetRequestId();
}

std::chrono::steady_clock::time_point RequestBase::GetRequestedTime() {
    return Mock_RequestBaseMock->GetRequestedTime();
}

} // namespace diagnostic_manager

#endif 