#include "RequestBase.hpp"

namespace diagnostic_manager {

std::atomic<uint64_t> RequestBase::next_request_id_(1U);

RequestBase::RequestBase() {
    request_id_ = next_request_id_.fetch_add(1);
    requested_time_ = std::chrono::steady_clock::now();
}
RequestBase::~RequestBase() {}

uint64_t RequestBase::GetRequestId() {
    return request_id_;
}

std::chrono::steady_clock::time_point RequestBase::GetRequestedTime() {
    return requested_time_;
}

} // namespace diagnostic_manager
