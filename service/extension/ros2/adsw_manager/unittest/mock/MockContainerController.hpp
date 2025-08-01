#ifndef MOCK_CONTAINER_CONTROLLER_HPP_
#define MOCK_CONTAINER_CONTROLLER_HPP_

#include "control/ContainerController.hpp"
#include <gmock/gmock.h>

namespace ros2 {
namespace adsw_manager {

class MockContainerController : public ContainerController {
public:
    MockContainerController(const std::string& deployment_file_path = "", const std::string& pod_name = "", const std::string& namespace_ = "", const std::string& container_name = "")
        : ContainerController(deployment_file_path, pod_name, namespace_, container_name) {}

    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(bool, StartContainer, (const std::string& container_name), ());
    MOCK_METHOD(bool, StopContainer, (const std::string& container_name), ());
    MOCK_METHOD(bool, GetContainerStatus, (const std::string& container_name), ());
    MOCK_METHOD(bool, GetContainerLogs, (const std::string& container_name), ());
    MOCK_METHOD(std::string, BaseCommand, (), ());
    MOCK_METHOD(std::string, BaseRos2Command, (), ());
    MOCK_METHOD(bool, ContainerSupported, (), ());
};

MockContainerController* Mock_ContainerController;

ContainerController::ContainerController(const std::string& deployment_file_path, const std::string& pod_name, const std::string& namespace_, const std::string& container_name)
{
    Mock_ContainerController = new MockContainerController(deployment_file_path, pod_name, namespace_, container_name);
}

ContainerController::~ContainerController() {
    Mock_ContainerController = nullptr;
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

std::string ContainerController::BaseCommand() {
    return Mock_ContainerController->BaseCommand();
}

std::string ContainerController::BaseRos2Command() {
    return Mock_ContainerController->BaseRos2Command();
}

bool ContainerController::ContainerSupported() {
    return Mock_ContainerController->ContainerSupported();
}


} // namespace adsw_manager
} // namespace ros2

#endif // MOCK_CONTAINER_CONTROLLER_HPP_
