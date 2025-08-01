
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_ROS_SERVICE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_ROS_SERVICE_CONTROLLER_HPP_

#include <gmock/gmock.h>
#include "control/ROSServiceController.hpp"


using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {


class MockROSServiceController : public ROSServiceController {
  public:
    MockROSServiceController() : ROSServiceController() {}
    virtual ~MockROSServiceController() {}
    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(bool, StartGroup, (const std::string& group), ());
    MOCK_METHOD(bool, StopGroup, (const std::string& group), ());
    MOCK_METHOD(bool, StartService, (const std::string& service_name), ());
    MOCK_METHOD(bool, StopService, (const std::string& service_name), ());
    MOCK_METHOD(bool, StartService, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(bool, StopService, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(bool, ProcessServiceStartRequest, (lifecyclemgr::StartServiceRequest* message), ());
    MOCK_METHOD(bool, ProcessServiceStopRequest, (lifecyclemgr::StopServiceRequest* message), ());
    MOCK_METHOD(bool, GetNodeLists, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(uint64_t, GetPID, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(std::shared_ptr<ContainerController>, GetContainer, (), ());
    MOCK_METHOD(std::shared_ptr<ROS2NodeController>, GetROS2NodeController, (), ());

};

MockROSServiceController* Mock_ROSServiceController;

ROSServiceController::ROSServiceController() {
    Mock_ROSServiceController = new MockROSServiceController();
    ros2node_controller_ = std::make_shared<ROS2NodeController>();
    container_controller_ = std::make_shared<ContainerController>("", "", "", "");
}
ROSServiceController::~ROSServiceController() {
    Mock_ROSServiceController = nullptr;
    ros2node_controller_ = nullptr;
    container_controller_ = nullptr;
}
bool ROSServiceController::Initialize() {
    return Mock_ROSServiceController->Initialize();
}
void ROSServiceController::Finalize() {
    Mock_ROSServiceController->Finalize();
    ros2node_controller_ = nullptr;
    container_controller_ = nullptr;
}
bool ROSServiceController::StartGroup(const std::string& group) {
    return Mock_ROSServiceController->StartGroup(group);
}
bool ROSServiceController::StopGroup(const std::string& group) {
    return Mock_ROSServiceController->StopGroup(group);
}
bool ROSServiceController::StartService(const std::string& service_name) {
    return Mock_ROSServiceController->StartService(service_name);
}
bool ROSServiceController::StopService(const std::string& service_name) {
    return Mock_ROSServiceController->StopService(service_name);
}
bool ROSServiceController::StartService(std::shared_ptr<Service> service) {
    return Mock_ROSServiceController->StartService(service);
}
bool ROSServiceController::StopService(std::shared_ptr<Service> service) {
    return Mock_ROSServiceController->StopService(service);
}
bool ROSServiceController::ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message) {
    return Mock_ROSServiceController->ProcessServiceStartRequest(message);
}
bool ROSServiceController::ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message) {
    return Mock_ROSServiceController->ProcessServiceStopRequest(message);
}
bool ROSServiceController::GetNodeLists(std::shared_ptr<Service> service) {
    return Mock_ROSServiceController->GetNodeLists(service);
}
uint64_t ROSServiceController::GetPID(std::shared_ptr<Service> service) {
    return Mock_ROSServiceController->GetPID(service);
}
std::shared_ptr<ContainerController> ROSServiceController::GetContainer() {
    return Mock_ROSServiceController->GetContainer();
}

std::shared_ptr<ROS2NodeController> ROSServiceController::GetROS2NodeController() {
    return Mock_ROSServiceController->GetROS2NodeController();
}



}
}

#endif