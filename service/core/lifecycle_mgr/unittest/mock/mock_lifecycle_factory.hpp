#ifndef MOCK_LIFECYCLE_FACTORY_HPP_
#define MOCK_LIFECYCLE_FACTORY_HPP_

#include "manager/LifecycleFactory.hpp"
#include "state/StateMonitor.hpp"
#include "state/StateManager.hpp"
#include "service/ServiceRegistry.hpp"

namespace lifecycle_manager {
class MockLifecycleFactory : public LifecycleFactory {
public:
   // Only mock virtual methods


    MOCK_METHOD(std::shared_ptr<DDSTransmitter>, GetTransmitter, (), ());
    MOCK_METHOD(std::shared_ptr<StateManager>, GetStateMgr, (), ());
    MOCK_METHOD(std::shared_ptr<StateMonitor>, GetMonitor, (), ());
    MOCK_METHOD(std::shared_ptr<PolicyManager>, GetPolicy, (), ());
    MOCK_METHOD(std::shared_ptr<MWServiceManager>, GetMWService, (), ());
    MOCK_METHOD(std::shared_ptr<ServiceRegistry>, GetRegistry, (), ());

};

inline MockLifecycleFactory* Mock_LifecycleFactory = nullptr;

LifecycleFactory::LifecycleFactory() {}
// LifecycleFactory::~LifecycleFactory() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

std::shared_ptr<LifecycleManager> LifecycleFactory::GetManager(){
    return Mock_LifecycleFactory->GetManager();
}

std::shared_ptr<DDSTransmitter> LifecycleFactory::GetTransmitter(){
    return Mock_LifecycleFactory->GetTransmitter();
}
std::shared_ptr<StateManager> LifecycleFactory::GetStateMgr(){
    return Mock_LifecycleFactory->GetStateMgr();
}
std::shared_ptr<StateMonitor> LifecycleFactory::GetMonitor(){
    return Mock_LifecycleFactory->GetMonitor();
}
std::shared_ptr<PolicyManager> LifecycleFactory::GetPolicy(){
    return Mock_LifecycleFactory->GetPolicy();
}
std::shared_ptr<MWServiceManager> LifecycleFactory::GetMWService(){
    return Mock_LifecycleFactory->GetMWService();
}
std::shared_ptr<ServiceRegistry> LifecycleFactory::GetRegistry(){
    return Mock_LifecycleFactory->GetRegistry();
}

// F013 phantom-publisher guard — permissive no-op stubs for unit tests so
// endpoint fixtures don't drop their own injected samples. The real prefix-
// pinning logic lives in LifecycleFactory.cpp (exercised by integration tests).
void LifecycleFactory::SetKnownAdswPublisher(
        const eprosima::fastdds::dds::InstanceHandle_t&) {}
bool LifecycleFactory::IsKnownAdswPublisher(
        const eprosima::fastdds::dds::InstanceHandle_t&) {
    return true;
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_FACTORY_HPP_
