#include "DiagInfo.hpp"

namespace diagnostic_manager {

DiagInfo::DiagInfo() {}
DiagInfo::DiagInfo(const int32_t code, const std::string& name, DiagInfoStatus status, const std::string& message)
        : diag_code_(code), diag_name_(name), status_(status), message_(message) {
    update_time_ = std::chrono::steady_clock::now();
}

DiagInfo::~DiagInfo() {}

int32_t DiagInfo::GetDiagCode() {
    return diag_code_;
}

std::string DiagInfo::GetDiagName() {
    return diag_name_;
}

DiagInfoStatus DiagInfo::GetStatus() {
    return status_;
}

std::string DiagInfo::GetMessage() {
    return message_;
}

std::chrono::steady_clock::time_point DiagInfo::GetUpdateTime() {
    return update_time_;
}

bool DiagInfo::operator==(const DiagInfo& other) const {
    return diag_code_ == other.diag_code_ &&
            diag_name_ == other.diag_name_ &&
            status_ == other.status_ &&
            message_ == other.message_;
}

} // namespace diagnostic_manager
