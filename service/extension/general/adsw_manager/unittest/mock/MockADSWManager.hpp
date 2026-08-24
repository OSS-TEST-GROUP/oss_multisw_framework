#ifndef GENERAL_ADSW_MANAGER_UNITTEST_MOCK_ADSW_MANAGER_HPP_
#define GENERAL_ADSW_MANAGER_UNITTEST_MOCK_ADSW_MANAGER_HPP_

#include <gmock/gmock.h>
#include <memory>

#include "ADSWManager.hpp"

// ADSWManager 링크 심.
//
// ADSWManager 는 애플리케이션 루트 싱글턴이고 Initialize() 가 DDSTransmitter 를
// 직접 만든다. 실물을 링크하면 테스트가 DDS participant 를 띄우게 되므로,
// 여기서 모든 메서드를 정의해 링커가 이쪽을 집게 한다.
//
// ros2 판과 달리 general 의 SetShared() 는 private 이다. 프로덕션 헤더를
// 건드리지 않으려고 Get() 이 반환할 포인터를 이 심이 직접 들고 있고,
// 테스트는 아래 SetMockADSWManager() 로 갈아끼운다.
namespace general {
namespace adsw_manager {

class MockADSWManager : public ADSWManager {
public:
    MockADSWManager() = default;
    ~MockADSWManager() override = default;

    MOCK_METHOD(std::shared_ptr<DDSTransmitter>, GetTransmitter, (), ());
    MOCK_METHOD(std::shared_ptr<ServiceRegistry>, GetServiceRegistry, (), ());
    MOCK_METHOD(std::shared_ptr<ServiceMonitor>, GetServiceMonitor, (), ());
    MOCK_METHOD(std::shared_ptr<ContainerServiceController>, GetServiceController, (), ());
    MOCK_METHOD(const char*, GetTopic, (MessageType topic), ());
    MOCK_METHOD(void, ConfirmUpdateServiceList, (), (override));
};

MockADSWManager* Mock_ADSWManager = nullptr;

void SetMockADSWManager(MockADSWManager* m) { Mock_ADSWManager = m; }

ADSWManager::ADSWManager() {}
// ADSWManager::~ADSWManager() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

ADSWManager* ADSWManager::Get() { return Mock_ADSWManager; }

std::shared_ptr<DDSTransmitter> ADSWManager::GetTransmitter() {
    return Mock_ADSWManager->GetTransmitter();
}
std::shared_ptr<ServiceRegistry> ADSWManager::GetServiceRegistry() {
    return Mock_ADSWManager->GetServiceRegistry();
}
std::shared_ptr<ServiceMonitor> ADSWManager::GetServiceMonitor() {
    return Mock_ADSWManager->GetServiceMonitor();
}
std::shared_ptr<ContainerServiceController> ADSWManager::GetServiceController() {
    return Mock_ADSWManager->GetServiceController();
}
const char* ADSWManager::GetTopic(MessageType topic) {
    return Mock_ADSWManager->GetTopic(topic);
}
void ADSWManager::ConfirmUpdateServiceList() {
    Mock_ADSWManager->ConfirmUpdateServiceList();
}

// 나머지는 테스트 경로에서 불리지 않는다. 심볼만 만족시키는 no-op.
bool ADSWManager::Initialize() { return true; }
bool ADSWManager::Start() { return true; }
void ADSWManager::Stop() {}
void ADSWManager::Process() {}
void ADSWManager::SetState(AdswState state) { adsw_state_ = state; }
AdswState ADSWManager::GetState() { return adsw_state_; }
std::shared_ptr<ADSWManager> ADSWManager::getShared() { return nullptr; }
void ADSWManager::SetShared(std::shared_ptr<ADSWManager>) {}

// F013 phantom-publisher guard — 유닛테스트에서는 전부 통과시킨다.
void ADSWManager::SetKnownLifecyclePublisher(
        const eprosima::fastdds::dds::InstanceHandle_t&) {}
bool ADSWManager::IsKnownLifecyclePublisher(
        const eprosima::fastdds::dds::InstanceHandle_t&) { return true; }
bool ADSWManager::KnownLifecyclePublisherSet() { return true; }

// 준비/정리 헬퍼. Get() 이 null 을 반환하는 상태도 테스트해야 하므로
// Release() 로 명시적으로 떼어낼 수 있게 둔다.
class ScopedMockADSWManager {
public:
    ScopedMockADSWManager() {
        mgr_ = new ::testing::NiceMock<MockADSWManager>();
        SetMockADSWManager(mgr_);
    }
    ~ScopedMockADSWManager() {
        SetMockADSWManager(nullptr);
        delete mgr_;
    }
    MockADSWManager& operator*() const { return *mgr_; }
    MockADSWManager* operator->() const { return mgr_; }
    void Release() { SetMockADSWManager(nullptr); }
    void Restore() { SetMockADSWManager(mgr_); }
private:
    MockADSWManager* mgr_;
};

} // namespace adsw_manager
} // namespace general

#endif
