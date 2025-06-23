#pragma once

#include <gmock/gmock.h>
#include "service/ArgumentConfig.hpp"


namespace lifecycle_manager {

class MockArgumentConfig : public ArgumentConfig {
public:
    MockArgumentConfig(const std::string& name, const std::string& description, const std::string& type, const std::string& default_value)
        : ArgumentConfig(name, description, type, default_value) {}

    MOCK_METHOD(std::string, GetName, (), ());
    MOCK_METHOD(std::string, GetDescription, (), ());
    MOCK_METHOD(std::string, GetType, (), ());
    MOCK_METHOD(std::string, GetDefault, (), ());

};

 

MockArgumentConfig* Mock_ArgumentConfig;

ArgumentConfig::ArgumentConfig(const std::string& name, const std::string& description, const std::string& type, const std::string& default_value)
    : arg_name_(name), arg_description_(description), arg_type_(type), arg_default_(default_value) {}
ArgumentConfig::~ArgumentConfig() {}

std::string ArgumentConfig::GetName() {
    return Mock_ArgumentConfig->GetName();
}

std::string ArgumentConfig::GetDescription() {
    return Mock_ArgumentConfig->GetDescription();
}

std::string ArgumentConfig::GetType() {
    return Mock_ArgumentConfig->GetType();
}

std::string ArgumentConfig::GetDefault() const {
    return Mock_ArgumentConfig->GetDefault();
}

} // namespace lifecycle_manager
