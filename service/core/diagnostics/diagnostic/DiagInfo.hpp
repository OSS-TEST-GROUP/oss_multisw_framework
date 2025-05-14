#ifndef DIAGNOSTICS_DIAGNOSTIC_DIAG_INFO_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_DIAG_INFO_HPP_

#include <iostream>
#include <chrono>

namespace diagnostic_manager {

enum class DiagInfoStatus {
    UNKNOWN,
    PASS,
    FAIL,
    WARNING,
};

class DiagInfo {
  public:
    DiagInfo();
    DiagInfo(const int32_t code, const std::string& name, DiagInfoStatus status, const std::string& message);
    virtual ~DiagInfo();

    int32_t GetDiagCode();
    std::string GetDiagName();
    DiagInfoStatus GetStatus();
    std::string GetMessage();
    std::chrono::steady_clock::time_point GetUpdateTime();

  private:
    int32_t diag_code_;
    std::string diag_name_;
    DiagInfoStatus status_;
    std::string message_;
    std::chrono::steady_clock::time_point update_time_;
};

} // namespace diagnostic_manager

#endif
