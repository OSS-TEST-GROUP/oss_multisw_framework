#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_ADSW_MANAGER_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_ADSW_MANAGER_HPP_

#include <gmock/gmock.h>

#include "ADSWManager.hpp"

#include "MockTypeSupport.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

    // Mock ADSWManager
    class MockADSWManager : public ADSWManager {
    public:
        MockADSWManager() : ADSWManager() {}
        virtual ~MockADSWManager() {}

        MOCK_METHOD(bool, Initialize, (), ());
        //MOCK_METHOD(void, Finalize, (), ());
        MOCK_METHOD(bool, Start, (), ());
        MOCK_METHOD(void, Stop, (), ());
        MOCK_METHOD(void, Process, (), ());

        MOCK_METHOD(std::shared_ptr<DDSTransmitter>, GetTransmitter, (), ());
        MOCK_METHOD(std::shared_ptr<ServiceRegistry>, GetServiceRegistry, (), ());
        MOCK_METHOD(std::shared_ptr<ServiceMonitor>, GetServiceMonitor, (), ());
        MOCK_METHOD(std::shared_ptr<ROSServiceController>, GetServiceController, (), ());
        MOCK_METHOD(std::shared_ptr<ROSServiceDiscovery>, GetServiceDiscovery, (), ());
        MOCK_METHOD(const char*, GetTopic, (ros2::adsw_manager::MessageType topic), ());
        MOCK_METHOD(void, SetState, (ros2::adsw_manager::AdswState state), ());
        MOCK_METHOD(ros2::adsw_manager::AdswState, GetState, (), ());

        MOCK_METHOD(ADSWManager*, Get, (), ());
    
    public:

    };

    MockADSWManager* Mock_ADSWManager;

    std::shared_ptr<ADSWManager> shared_instance_;

    ADSWManager::ADSWManager() {
    }
    
    ADSWManager::~ADSWManager() {
    }
    bool ADSWManager::Initialize() {
        return Mock_ADSWManager->Initialize();
    }

    bool ADSWManager::Start() {
        return Mock_ADSWManager->Start();
    }
    void ADSWManager::Stop() {
        Mock_ADSWManager->Stop();
    }
    void ADSWManager::Process() {
        Mock_ADSWManager->Process();
    }

    std::shared_ptr<DDSTransmitter> ADSWManager::GetTransmitter() {
        return Mock_ADSWManager->GetTransmitter();
    }
    std::shared_ptr<ServiceRegistry> ADSWManager::GetServiceRegistry() {
        return Mock_ADSWManager->GetServiceRegistry();
    }
    std::shared_ptr<ServiceMonitor> ADSWManager::GetServiceMonitor() {
        return Mock_ADSWManager->GetServiceMonitor();
    }
    std::shared_ptr<ROSServiceController> ADSWManager::GetServiceController() {
        return Mock_ADSWManager->GetServiceController();
    }
    std::shared_ptr<ROSServiceDiscovery> ADSWManager::GetServiceDiscovery() {
        return Mock_ADSWManager->GetServiceDiscovery();
    }
    const char* ADSWManager::GetTopic(MessageType topic) {
        return Mock_ADSWManager->GetTopic(topic);
    }
    void ADSWManager::SetState(AdswState state) {
        Mock_ADSWManager->SetState(state);
    }
    AdswState ADSWManager::GetState() {
        return Mock_ADSWManager->GetState();
    }

    ADSWManager* ADSWManager::Get() {
        return shared_instance_.get();
    }

    void ADSWManager::SetShared(std::shared_ptr<ADSWManager> instance) {
        shared_instance_ = instance;
    }
}
}

#endif
