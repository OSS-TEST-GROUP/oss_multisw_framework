#ifndef DIAGNOSTICS_UNITTEST_MOCK_DIAG_INFO_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_DIAG_INFO_MOCK_HPP_

#include <gmock/gmock.h>
#include "diagnostic/DiagInfo.hpp"

namespace diagnostic_manager {

class DiagInfoMock : public DiagInfo {
    MOCK_METHOD0(GetDiagCode, int32_t());
    MOCK_METHOD0(GetDiagName, std::string());
    MOCK_METHOD0(GetStatus, DiagInfoStatus());
    MOCK_METHOD0(GetMessage, std::string());
    MOCK_METHOD0(GetUpdateTime, std::chrono::steady_clock::time_point());
};

DiagInfoMock* Mock_DiagInfo;

DiagInfo::DiagInfo() {}
DiagInfo::DiagInfo(const int32_t code, const std::string& name, DiagInfoStatus status, const std::string& message) {}

DiagInfo::~DiagInfo() {}

int32_t DiagInfo::GetDiagCode() {
    return Mock_DiagInfo->GetDiagCode();
}

std::string DiagInfo::GetDiagName() {
    return Mock_DiagInfo->GetDiagName();
}

DiagInfoStatus DiagInfo::GetStatus() {
    return Mock_DiagInfo->GetStatus();
}

std::string DiagInfo::GetMessage() {
    return Mock_DiagInfo->GetMessage();
}

std::chrono::steady_clock::time_point DiagInfo::GetUpdateTime() {
    return Mock_DiagInfo->GetUpdateTime();
}

bool DiagInfo::operator==(const DiagInfo& other) const {
    return false;
}

} // namespace diagnostic_manager

#endif 