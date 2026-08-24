#ifndef MOCK_CONTAINER_CONTROLLER_HPP_
#define MOCK_CONTAINER_CONTROLLER_HPP_

#include "control/ContainerController.hpp"
#include <gmock/gmock.h>

namespace ros2 {
namespace adsw_manager {

class MockContainerController : public ContainerController {
public:
    MockContainerController()
        : ContainerController() {}
    ~MockContainerController(){}

    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(bool, StartContainer, (const std::string& container_name), ());
    MOCK_METHOD(bool, StopContainer, (const std::string& container_name), ());
    MOCK_METHOD(bool, RestartContainer, (const std::string& container_name), ());
    MOCK_METHOD(bool, GetContainerStatus, (const std::string& container_name), ());
    MOCK_METHOD(bool, GetContainerLogs, (const std::string& container_name), ());
    MOCK_METHOD(std::string, GetContainerStatusStr, (const std::string& container_name), ());
    MOCK_METHOD(int32_t, GetContainerExitCode, (const std::string& container_name), ());
    MOCK_METHOD(bool, IsContainerRunning, (const std::string& container_name), ());
    MOCK_METHOD(std::vector<std::shared_ptr<Container>>, GetAllContainers, (), ());
    MOCK_METHOD(std::string, BaseCommand, (const std::string& container_name), ());
    MOCK_METHOD(std::string, BaseRos2Command, (const std::string& container_name), ());
    MOCK_METHOD(bool, ContainerSupported, (), ());
    MOCK_METHOD(std::shared_ptr<Container>, GetContainer, (const std::string& container_name), ());
    MOCK_METHOD(bool, AddContainer, (std::shared_ptr<Container> container), ());
};

MockContainerController* Mock_ContainerController;

ContainerController::ContainerController(){
}

// ~ContainerController() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.
bool ContainerController::Initialize() {
    return Mock_ContainerController->Initialize();
}
void ContainerController::Finalize() {
    Mock_ContainerController->Finalize();
}
bool ContainerController::StartContainer(const std::string& container_name) {
    return Mock_ContainerController->StartContainer(container_name);
}

bool ContainerController::StopContainer(const std::string& container_name) {
    return Mock_ContainerController->StopContainer(container_name);
}

bool ContainerController::RestartContainer(const std::string& container_name) {
    return Mock_ContainerController->RestartContainer(container_name);
}

bool ContainerController::GetContainerStatus(const std::string& container_name) {
    return Mock_ContainerController->GetContainerStatus(container_name);
}

bool ContainerController::GetContainerLogs(const std::string& container_name) {
    return Mock_ContainerController->GetContainerLogs(container_name);
}

std::string ContainerController::GetContainerStatusStr(const std::string& container_name) {
    return Mock_ContainerController->GetContainerStatusStr(container_name);
}

int32_t ContainerController::GetContainerExitCode(const std::string& container_name) {
    return Mock_ContainerController->GetContainerExitCode(container_name);
}

bool ContainerController::IsContainerRunning(const std::string& container_name) {
    return Mock_ContainerController->IsContainerRunning(container_name);
}

std::vector<std::shared_ptr<Container>> ContainerController::GetAllContainers() {
    return Mock_ContainerController->GetAllContainers();
}

std::string ContainerController::BaseCommand(const std::string& container_name) {
    return Mock_ContainerController->BaseCommand(container_name);
}

std::string ContainerController::BaseRos2Command(const std::string& container_name) {
    return Mock_ContainerController->BaseRos2Command(container_name);
}

bool ContainerController::ContainerSupported() {
    return Mock_ContainerController->ContainerSupported();
}

std::shared_ptr<Container> ContainerController::GetContainer(const std::string& container_name) {
    return Mock_ContainerController->GetContainer(container_name);
}
bool ContainerController::AddContainer(const std::shared_ptr<Container>& container) {
    return Mock_ContainerController->AddContainer(container);
}

// 외부 프로세스 실행 seam. virtual 이라 vtable 이 정의를 요구한다.
// 이 심을 쓰는 테스트는 ContainerController 를 통째로 mock 으로 대체하므로
// 여기까지 내려올 일이 없다. 혹시 내려오더라도 docker 를 부르면 안 되니
// "아무것도 실행하지 않았고 출력도 없다" 로 둔다.
int ContainerController::RunCommand(const std::string&) {
    return -1;
}
std::string ContainerController::CaptureCommand(const std::string&, size_t) {
    return "";
}

} // namespace adsw_manager
} // namespace ros2

#endif // MOCK_CONTAINER_CONTROLLER_HPP_
