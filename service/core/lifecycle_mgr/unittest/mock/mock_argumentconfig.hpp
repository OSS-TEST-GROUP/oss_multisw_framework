#ifndef MOCK_LIFECYCLE_ARGUMENT_CONFIG_HPP_
#define MOCK_LIFECYCLE_ARGUMENT_CONFIG_HPP_

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
    MOCK_METHOD(std::string, GetArgument, (), ());

};

MockArgumentConfig* Mock_ArgumentConfig;

ArgumentConfig::ArgumentConfig(const std::string& name, const std::string& description, const std::string& type, const std::string& default_value)
    : arg_name_(name), arg_description_(description), arg_type_(type), arg_default_(default_value) {}
// ArgumentConfig::~ArgumentConfig() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

std::string ArgumentConfig::GetName() {
    return Mock_ArgumentConfig->GetName();
}

std::string ArgumentConfig::GetDescription() {
    return Mock_ArgumentConfig->GetDescription();
}

std::string ArgumentConfig::GetType() {
    return Mock_ArgumentConfig->GetType();
}

std::string ArgumentConfig::GetArgument() {
    return Mock_ArgumentConfig->GetArgument();
}

} // namespace lifecycle_manager


#endif // MOCK_LIFECYCLE_ARGUMENT_CONFIG_HPP_