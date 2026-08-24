#include "MockADSWManager.hpp"
#include <iostream>

namespace ros2 {
namespace adsw_manager {

    MockADSWManager* Mock_ADSWManager = nullptr;

    // Define the shared instance
    std::shared_ptr<ADSWManager> shared_instance_;

    // Implement all ADSWManager methods to delegate to the mock
    ADSWManager::ADSWManager() {
        std::cerr << "MockADSWManager::ADSWManager() constructor called" << std::endl;
    }
    
    // ~ADSWManager() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
    // 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
    // defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.
    
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
        // Always return shared_instance directly
        // The test setup calls SetShared() with the mock instance
        auto result = shared_instance_.get();
        std::cerr << "MockADSWManager::Get() called - shared_instance_.get() = " << result 
                  << ", shared_instance_.use_count() = " << shared_instance_.use_count() << std::endl;
        return result;
    }

    void ADSWManager::ConfirmUpdateServiceList() {
        Mock_ADSWManager->ConfirmUpdateServiceList();
    }

    void ADSWManager::SetShared(std::shared_ptr<ADSWManager> instance) {
        std::cerr << "MockADSWManager::SetShared() called with " << (instance ? "valid" : "NULL") << " instance" << std::endl;
        shared_instance_ = instance;
    }

    // F013 phantom-publisher guard — permissive no-op stubs for unit tests so
    // endpoints don't drop their own injected samples. Real prefix-pinning logic
    // lives in ADSWManager.cpp (exercised by integration tests).
    void ADSWManager::SetKnownLifecyclePublisher(
            const eprosima::fastdds::dds::InstanceHandle_t&) {}
    bool ADSWManager::IsKnownLifecyclePublisher(
            const eprosima::fastdds::dds::InstanceHandle_t&) {
        return true;
    }

} // namespace adsw_manager
} // namespace ros2
