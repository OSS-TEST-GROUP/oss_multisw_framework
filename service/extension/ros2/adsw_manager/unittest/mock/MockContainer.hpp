#ifndef MOCK_CONTAINER_HPP_
#define MOCK_CONTAINER_HPP_

#include <gmock/gmock.h>
#include "service/Container.hpp"

namespace ros2 {
namespace adsw_manager {

class MockContainer : public Container {
public:
    MockContainer()
        : Container("mock_name", "mock_image", "mock_deploy_file", "mock_namespace", ContainerType::DOCKER) {}
    ~MockContainer() override = default;

    MOCK_METHOD(std::string, GetName, (), (const));
    MOCK_METHOD(std::string, GetImage, (), (const));
    MOCK_METHOD(std::string, GetDeployFile, (), (const));
    MOCK_METHOD(std::string, GetNamespace, (), (const));
    MOCK_METHOD(std::string, GetPodName, (), (const));

};


MockContainer* Mock_Container;

Container::Container(const std::string& name, const std::string& image, const std::string& deploy_file, const std::string& namespace_, ContainerType type)
    : name_(name),
      image_(image),
      deploy_file_(deploy_file),
      namespace_(namespace_),
      container_type_(type) {
}

// ~Container() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.
// Mock_Container 가 세팅됐을 때만 목으로 넘기고, 아니면 생성자가 채운 실제
// 멤버를 그대로 돌려준다 — MockService.hpp 가 Mock_Service 에 쓰는 것과 같은
// 규약이다.
//
// 왜 바꿨나: 원래는 무조건 Mock_Container 를 역참조했는데, 이 저장소의 어떤
// 테스트도 Mock_Container 를 세팅하지 않는다(전부 nullptr). 그래서 진짜
// Container 객체를 만들어 쓰는 순간 널 역참조로 죽었고, 결국
// ServiceRegistry::Save() 의 컨테이너 메타데이터 직렬화(image/container-type/
// deploy-filepath/extra_args/command_args)를 아무도 실행시킬 수 없었다.
// 그 블록이 커버리지 0 이었던 이유가 이것이다.
// 목을 세팅한 기존 사용처는 없으므로 동작이 바뀌는 경로도 없다.
std::string Container::GetName() const {
    if (Mock_Container) return Mock_Container->GetName();
    return name_;
}
std::string Container::GetImage() const {
    if (Mock_Container) return Mock_Container->GetImage();
    return image_;
}
std::string Container::GetDeployFile() const {
    if (Mock_Container) return Mock_Container->GetDeployFile();
    return deploy_file_;
}
std::string Container::GetNamespace() const {
    if (Mock_Container) return Mock_Container->GetNamespace();
    return namespace_;
}
std::string Container::GetPodName() const {
    if (Mock_Container) return Mock_Container->GetPodName();
    return name_;   // Container.cpp 의 실제 구현과 동일
}
// F013: container-type accessor — backed by the real member set in the ctor.
ContainerType Container::GetContainerType() const {
    return container_type_;
}

// extra/command args 접근자 — 위와 같이 실제 멤버를 그대로 쓴다.
// ServiceRegistry::Save / LoadServiceSaved 가 호출하므로 없으면
// ADSWServiceRegistryTest 가 링크되지 않는다.
void Container::SetExtraArgs(const std::vector<std::string>& args) {
    extra_args_ = args;
}
const std::vector<std::string>& Container::GetExtraArgs() const {
    return extra_args_;
}
void Container::SetCommandArgs(const std::vector<std::string>& args) {
    command_args_ = args;
}
const std::vector<std::string>& Container::GetCommandArgs() const {
    return command_args_;
}


} // namespace adsw_manager
} // namespace ros2

#endif // MOCK_CONTAINER_HPP_
