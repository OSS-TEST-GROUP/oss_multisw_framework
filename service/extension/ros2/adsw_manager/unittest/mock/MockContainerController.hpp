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
    MOCK_METHOD(bool, GetContainerStatus, (const std::string& container_name), ());
    MOCK_METHOD(bool, GetContainerLogs, (const std::string& container_name), ());
    MOCK_METHOD(std::string, BaseCommand, (const std::string& container_name), ());
    MOCK_METHOD(std::string, BaseRos2Command, (const std::string& container_name), ());
    MOCK_METHOD(bool, ContainerSupported, (), ());
    MOCK_METHOD(std::shared_ptr<Container>, GetContainer, (const std::string& container_name), ());
    MOCK_METHOD(bool, AddContainer, (std::shared_ptr<Container> container), ());
};

MockContainerController* Mock_ContainerController;

ContainerController::ContainerController(){
}

ContainerController::~ContainerController() {
}
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

bool ContainerController::GetContainerStatus(const std::string& container_name) {
    return Mock_ContainerController->GetContainerStatus(container_name);
}

bool ContainerController::GetContainerLogs(const std::string& container_name) {
    return Mock_ContainerController->GetContainerLogs(container_name);
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
bool ContainerController::AddContainer(std::shared_ptr<Container> container) {
    return Mock_ContainerController->AddContainer(container);
}

} // namespace adsw_manager
} // namespace ros2

#endif // MOCK_CONTAINER_CONTROLLER_HPP_
