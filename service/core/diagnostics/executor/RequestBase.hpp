#ifndef DIAGNOSTICS_EXECUTOR_REQUEST_BASE_HPP_
#define DIAGNOSTICS_EXECUTOR_REQUEST_BASE_HPP_

#include <iostream>
#include <atomic>
#include <chrono>

namespace diagnostic_manager {

class RequestBase {
  public:
    RequestBase();
    virtual ~RequestBase();

    uint64_t GetRequestId();
    std::chrono::steady_clock::time_point GetRequestedTime();

  private:
    uint64_t request_id_;
    static std::atomic<uint64_t> next_request_id_;
    std::chrono::steady_clock::time_point requested_time_;
};

} // namespace diagnostic_manager

#endif
