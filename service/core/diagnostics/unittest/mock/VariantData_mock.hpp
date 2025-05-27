#ifndef DIAGNOSTICS_UNITTEST_MOCK_VARIANT_DATA_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_VARIANT_DATA_MOCK_HPP_

#include <gmock/gmock.h>
#include "executor/ServiceTask.hpp"

namespace diagnostic_manager {

class VariantDataMock {
    MOCK_METHOD1(SetName, void(const std::string&));
    MOCK_METHOD0(GetName, std::string());
    MOCK_METHOD1(GetValueType, std::type_index(const std::string&));
    MOCK_METHOD1(HasKey, bool(const std::string& name));
    MOCK_METHOD0(GetKeyList, std::vector<std::string>());
    MOCK_METHOD0(GetValueNum, uint64_t());
};

VariantDataMock* Mock_VariantDataMock;

void VariantData::SetName(const std::string& name) {
    Mock_VariantDataMock->SetName(name);
}

std::string VariantData::GetName() {
    return Mock_VariantDataMock->GetName();
}

std::type_index VariantData::GetValueType(const std::string& name) {
    return Mock_VariantDataMock->GetValueType(name);
}

bool VariantData::HasKey(const std::string& name) {
    return Mock_VariantDataMock->HasKey(name);
}

std::vector<std::string> VariantData::GetKeyList() {
    return Mock_VariantDataMock->GetKeyList();
}

uint64_t VariantData::GetValueNum() {
    return Mock_VariantDataMock->GetValueNum();
}

} // namespace diagnostic_manager

#endif 