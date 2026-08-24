// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ADSWManager.hpp"

#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockServiceMonitor.hpp"
#include "mock/MockROSServiceController.hpp"
#include "mock/MockEndPointFactory.hpp"
#include "mock/MockServiceMonitor.hpp"
#include "mock/MockContainerHealthMonitor.hpp"

#include "mock/MockLogger.hpp"


using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;
using ::testing::NiceMock;

using namespace ros2::adsw_manager;


class ADSWManagerTest : public ::testing::Test {
protected:
    ros2::adsw_manager::ADSWManager manager;

    void SetUp() override {
        Mock_DDSTransmitter = new NiceMock<MockDDSTransmitter>();
        Mock_ServiceRegistry = new NiceMock<MockServiceRegistry>();
        Mock_ServiceMonitor = new NiceMock<MockServiceMonitor>();
        Mock_ROSServiceController = new NiceMock<MockROSServiceController>();
        Mock_EndPointFactory = new NiceMock<MockEndPointFactory>();

        // Wrap mocks in shared_ptr with no-op deleters (we manage lifetime in TearDown)
        manager.dds_transmitter_ = std::shared_ptr<MockDDSTransmitter>(Mock_DDSTransmitter, [](MockDDSTransmitter*){});
        manager.service_registry_ = std::shared_ptr<MockServiceRegistry>(Mock_ServiceRegistry, [](MockServiceRegistry*){});
        manager.service_monitor_ = std::shared_ptr<MockServiceMonitor>(Mock_ServiceMonitor, [](MockServiceMonitor*){});
        manager.service_controller_ = std::shared_ptr<MockROSServiceController>(Mock_ROSServiceController, [](MockROSServiceController*){});
        manager.endpoint_factory_ = std::shared_ptr<MockEndPointFactory>(Mock_EndPointFactory, [](MockEndPointFactory*){});

        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
           
        // Setup default mock expectations
        ON_CALL(*Mock_EndPointFactory, GetTopic(_)).WillByDefault(Return("mock_topic"));
        ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(true));

        // ADSWManager::Initialize() 는 하위 4개의 Initialize() 가 모두 true 여야
        // true 를 돌려준다. NiceMock 의 bool 기본값은 false 라 이걸 프로그램하지
        // 않으면 첫 줄 `if (!dds_transmitter_->Initialize()) return false;` 에서
        // 바로 실패한다. 이 테스트들은 "하위 시스템이 정상일 때 Initialize 가
        // 성공한다" 를 검증하려는 것이므로 정상 경로를 깔아준다.
        ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
        ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
        ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(true));
        ON_CALL(*Mock_ROSServiceController, Initialize()).WillByDefault(Return(true));
    }

    void TearDown() override {
        // Reset the manager's shared_ptrs first to release references
        manager.dds_transmitter_ = nullptr;
        manager.service_registry_ = nullptr;
        manager.service_monitor_ = nullptr;
        manager.service_controller_ = nullptr;
        manager.endpoint_factory_ = nullptr;
        
        // Clear the singleton instance
        ADSWManager::SetShared(nullptr);
        
        // Delete the mock objects (they have no-op deleters in shared_ptr)
        delete Mock_DDSTransmitter;
        Mock_DDSTransmitter = nullptr;
        delete Mock_ServiceRegistry;
        Mock_ServiceRegistry = nullptr;
        delete Mock_ServiceMonitor;
        Mock_ServiceMonitor = nullptr;
        delete Mock_ROSServiceController;
        Mock_ROSServiceController = nullptr;
        delete Mock_EndPointFactory;
        Mock_EndPointFactory = nullptr;
        
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

};

TEST_F(ADSWManagerTest, InitializeAndStartStop) {
    ASSERT_TRUE(manager.Initialize());
    ASSERT_TRUE(manager.Start());
    manager.Stop();
}

TEST_F(ADSWManagerTest, Getters) {
    EXPECT_NE(manager.GetTransmitter(), nullptr);
    EXPECT_NE(manager.GetServiceRegistry(), nullptr);
    EXPECT_NE(manager.GetServiceMonitor(), nullptr);
    EXPECT_NE(manager.GetServiceController(), nullptr);
    // ServiceDiscovery may be null or not, depending on mock
    SUCCEED();
}

TEST_F(ADSWManagerTest, GetTopic) {
    EXPECT_STREQ(manager.GetTopic(ros2::adsw_manager::MessageType::START_SERVICE_REQUEST), "mock_topic");
}

TEST_F(ADSWManagerTest, SingletonGetShared) {
    auto shared1 = ros2::adsw_manager::ADSWManager::getShared();
    auto shared2 = ros2::adsw_manager::ADSWManager::getShared();
    EXPECT_EQ(shared1.get(), shared2.get());
    EXPECT_EQ(ros2::adsw_manager::ADSWManager::Get(), shared1.get());
}

TEST_F(ADSWManagerTest, InitializeMultipleTimes) {
    ASSERT_TRUE(manager.Initialize());
    // Second initialize should also succeed (idempotent)
    ASSERT_TRUE(manager.Initialize());
}

TEST_F(ADSWManagerTest, StartWithoutInitialize) {
    // Create a fresh manager without initialization
    ros2::adsw_manager::ADSWManager fresh_manager;
    // Start may succeed or fail, but should not crash
    EXPECT_NO_THROW(fresh_manager.Start());
}

TEST_F(ADSWManagerTest, StopWithoutStart) {
    // Stop without start should not crash
    EXPECT_NO_THROW(manager.Stop());
}

TEST_F(ADSWManagerTest, GetTopicAllMessageTypes) {
    // Test GetTopic for various message types
    EXPECT_NO_THROW(manager.GetTopic(ros2::adsw_manager::MessageType::START_SERVICE_REQUEST));
    EXPECT_NO_THROW(manager.GetTopic(ros2::adsw_manager::MessageType::STOP_SERVICE_REQUEST));
    EXPECT_NO_THROW(manager.GetTopic(ros2::adsw_manager::MessageType::GET_SERVICE_STATE_REQUEST));
}

TEST_F(ADSWManagerTest, GetServiceDiscovery) {
    auto discovery = manager.GetServiceDiscovery();
    // May be null depending on mock setup
    SUCCEED();
}

TEST_F(ADSWManagerTest, StartAndStopMultipleTimes) {
    ASSERT_TRUE(manager.Initialize());
    ASSERT_TRUE(manager.Start());
    manager.Stop();
    
    // Should be able to start and stop again
    ASSERT_TRUE(manager.Start());
    manager.Stop();
}

// ==================== Additional tests for improved coverage ====================

// Test Initialize succeeds when all components initialize successfully
TEST_F(ADSWManagerTest, Initialize_AllComponentsSucceed) {
    // Setup: All Initialize() calls return true
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ROSServiceController, Initialize()).WillByDefault(Return(true));
    
    EXPECT_TRUE(manager.Initialize());
}

// Test Initialize fails when DDSTransmitter::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_DDSTransmitterFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Initialize fails when EndPointFactory::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_EndPointFactoryFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Initialize fails when ServiceMonitor::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_ServiceMonitorFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Initialize fails when ROSServiceController::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_ServiceControllerFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ROSServiceController, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Start when endpoint_factory_ is not null
TEST_F(ADSWManagerTest, Start_WithEndpointFactory) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ROSServiceController, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(true));
    
    EXPECT_CALL(*Mock_EndPointFactory, CreateEndPoints(_)).Times(1);
    
    manager.Initialize();
    EXPECT_TRUE(manager.Start());
}

// Test Start when service_registry_->LoadServiceList() fails
TEST_F(ADSWManagerTest, Start_LoadServiceListFails) {
    ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Start());
}

// Test Start with null endpoint_factory_
TEST_F(ADSWManagerTest, Start_NullEndpointFactory) {
    manager.endpoint_factory_ = nullptr;
    ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(true));
    
    // Should not call CreateEndPoints and should still succeed
    EXPECT_TRUE(manager.Start());
}

// Test Process() function
// ── Process() 와 ConfirmUpdateServiceList() 의 역할 분담 ───────────────────
//
// 아래 두 테스트는 원래 "Process() 가 ADSW_STARTED 에서 RUNNING 으로 전이하고
// NotifyAdswStatus 를 부른다" 를 검증했다. F013 리팩터링으로 그 동작이
// ConfirmUpdateServiceList() (UpdateServiceListResponse 수신 경로) 로 옮겨갔고
// Process() 는 요청 재전송만 담당하게 됐는데, 테스트는 따라가지 못해 계속
// 실패하고 있었다. 유닛 레인(integration-tests.regex 로 제외)과 통합 레인
// (runnable 목록 미포함) 양쪽에서 빠져 있어 드러나지 않았을 뿐이다.
//
// Process() 의 현재 계약을 고정하고, 옮겨간 동작은 아래 ConfirmUpdateServiceList
// 테스트로 되살린다 (그쪽은 그때까지 테스트가 하나도 없었다).

TEST_F(ADSWManagerTest, Process_InStartedState_SendsRequestAndKeepsState) {
    manager.SetState(AdswState::ADSW_STARTED);

    // ADSW_STARTED 에서는 UpdateServiceListRequest 를 보내는 것까지가 Process 의 몫.
    EXPECT_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest())
        .Times(1).WillOnce(Return(true));
    // 상태 전이와 상태 통지는 응답 수신 시점의 일이라 여기서 일어나면 안 된다.
    EXPECT_CALL(*Mock_ServiceMonitor, NotifyAdswStatus(_, _)).Times(0);

    EXPECT_NO_THROW(manager.Process());

    EXPECT_EQ(manager.GetState(), AdswState::ADSW_STARTED);
}

// Test Process() when ProcessUpdateServiceListRequest returns false
TEST_F(ADSWManagerTest, Process_UpdateServiceListRequestFails_KeepsState) {
    manager.SetState(AdswState::ADSW_STARTED);

    EXPECT_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest())
        .Times(1).WillOnce(Return(false));
    EXPECT_CALL(*Mock_ServiceMonitor, NotifyAdswStatus(_, _)).Times(0);

    EXPECT_NO_THROW(manager.Process());

    EXPECT_EQ(manager.GetState(), AdswState::ADSW_STARTED);
}

TEST_F(ADSWManagerTest, ConfirmUpdateServiceList_FromStarted_TransitionsToRunning) {
    manager.SetState(AdswState::ADSW_STARTED);

    EXPECT_CALL(*Mock_ServiceRegistry, UpdateLastSentRecovery()).Times(1);
    EXPECT_CALL(*Mock_ServiceMonitor, NotifyAdswStatus(1, _)).Times(1);

    EXPECT_NO_THROW(manager.ConfirmUpdateServiceList());

    EXPECT_EQ(manager.GetState(), AdswState::ADSW_RUNNING);
}

TEST_F(ADSWManagerTest, ConfirmUpdateServiceList_FromRunning_StaysRunningWithoutNotify) {
    manager.SetState(AdswState::ADSW_RUNNING);

    EXPECT_CALL(*Mock_ServiceRegistry, UpdateLastSentRecovery()).Times(1);
    // 이미 RUNNING 이면 재동기화일 뿐이라 상태 통지를 다시 보내지 않는다.
    EXPECT_CALL(*Mock_ServiceMonitor, NotifyAdswStatus(_, _)).Times(0);

    EXPECT_NO_THROW(manager.ConfirmUpdateServiceList());

    EXPECT_EQ(manager.GetState(), AdswState::ADSW_RUNNING);
}

// Test Process() when service_monitor_ is null
TEST_F(ADSWManagerTest, Process_NullServiceMonitor) {
    manager.SetState(AdswState::ADSW_STARTED);
    manager.service_monitor_ = nullptr;
    
    ON_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest()).WillByDefault(Return(true));
    
    // Should not crash even with null service_monitor_
    EXPECT_NO_THROW(manager.Process());
}

// Test Process() when not in ADSW_STARTED state
TEST_F(ADSWManagerTest, Process_NotInStartedState) {
    manager.SetState(AdswState::ADSW_INIT);
    
    // Process should do nothing when not in STARTED state
    EXPECT_NO_THROW(manager.Process());
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_INIT);
}

// Test GetState()
TEST_F(ADSWManagerTest, GetState_ReturnsCorrectState) {
    manager.SetState(AdswState::ADSW_INIT);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_INIT);
    
    manager.SetState(AdswState::ADSW_STARTED);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_STARTED);
    
    manager.SetState(AdswState::ADSW_RUNNING);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_RUNNING);
    
    manager.SetState(AdswState::ADSW_TERMINATE);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_TERMINATE);
}

// Test GetServiceDiscovery when service_monitor_ is null
TEST_F(ADSWManagerTest, GetServiceDiscovery_NullServiceMonitor) {
    manager.service_monitor_ = nullptr;
    
    auto discovery = manager.GetServiceDiscovery();
    EXPECT_EQ(discovery, nullptr);
}

// Test SetShared()
TEST_F(ADSWManagerTest, SetShared) {
    auto custom_manager = std::make_shared<ADSWManager>();
    
    ADSWManager::SetShared(custom_manager);
    
    // Verify that SetShared was called (we can't easily verify the internal state,
    // but we ensure it doesn't crash)
    SUCCEED();
}

// Test Stop with null components
TEST_F(ADSWManagerTest, Stop_NullComponents) {
    manager.service_controller_ = nullptr;
    manager.service_monitor_ = nullptr;
    manager.dds_transmitter_ = nullptr;
    
    // Should not crash when components are null
    EXPECT_NO_THROW(manager.Stop());
}

// Test Stop calls Finalize on all components
TEST_F(ADSWManagerTest, Stop_CallsFinalize) {
    EXPECT_CALL(*Mock_ROSServiceController, Finalize()).Times(1);
    EXPECT_CALL(*Mock_ServiceMonitor, Finalize()).Times(1);
    EXPECT_CALL(*Mock_DDSTransmitter, Finalize()).Times(1);
    
    manager.Stop();
    
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_TERMINATE);
}


// ============================================================================
// Process() 의 재전송 / 재동기화 타이밍
//
// 기존 테스트는 "아직 한 번도 안 보낸" 첫 전송만 밟았다. 경과 시간에 걸린
// 두 분기 — ADSW_STARTED 의 3초 재시도, ADSW_RUNNING 의 10초 재동기화 —
// 는 실행 0회였다. steady_clock 을 기다릴 수는 없으니 마지막 전송 시각을
// 과거로 밀어 넣는다 (-fno-access-control 로 private 에 직접 접근한다,
// 이 픽스처가 이미 dds_transmitter_ 등에 쓰는 방식과 같다).
// ============================================================================

namespace {
// 마지막 전송이 ms 밀리초 전이었던 상태로 만든다.
void PretendLastSentWas(ros2::adsw_manager::ADSWManager& m, int ms) {
    m.last_update_list_sent_ = std::chrono::steady_clock::now()
                             - std::chrono::milliseconds(ms);
}
} // namespace

TEST_F(ADSWManagerTest, Process_InStartedState_ResendsAfterRetryInterval) {
    manager.SetState(AdswState::ADSW_STARTED);
    manager.update_list_sent_ = true;               // 이미 한 번 보냈고
    PretendLastSentWas(manager, 4000);              // 응답 없이 4초 지났다

    EXPECT_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest())
        .WillOnce(Return(true));

    manager.Process();

    EXPECT_EQ(manager.GetState(), AdswState::ADSW_STARTED)
        << "재전송만 해야 하는데 상태가 바뀌었다";
}

TEST_F(ADSWManagerTest, Process_InStartedState_QuietBeforeRetryInterval) {
    manager.SetState(AdswState::ADSW_STARTED);
    manager.update_list_sent_ = true;
    PretendLastSentWas(manager, 100);               // 아직 3초가 안 됐다

    EXPECT_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest()).Times(0);

    manager.Process();
}

TEST_F(ADSWManagerTest, Process_InStartedState_FailedResendDoesNotStampTime) {
    manager.SetState(AdswState::ADSW_STARTED);
    manager.update_list_sent_ = true;
    PretendLastSentWas(manager, 4000);
    const auto before = manager.last_update_list_sent_;

    EXPECT_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest())
        .WillOnce(Return(false));

    manager.Process();

    // 실패했으면 시각을 갱신하면 안 된다 — 갱신하면 다음 3초를 통째로 쉰다.
    EXPECT_EQ(manager.last_update_list_sent_, before);
}

TEST_F(ADSWManagerTest, Process_InRunningState_ResyncsAfterInterval) {
    // lifecycle_mgr 가 재시작했을 수 있으므로 주기적으로 목록을 다시 보낸다.
    manager.SetState(AdswState::ADSW_RUNNING);
    PretendLastSentWas(manager, 11000);             // 10초 경과

    EXPECT_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest())
        .WillOnce(Return(true));

    manager.Process();

    EXPECT_EQ(manager.GetState(), AdswState::ADSW_RUNNING);
}

TEST_F(ADSWManagerTest, Process_InRunningState_QuietBeforeInterval) {
    manager.SetState(AdswState::ADSW_RUNNING);
    PretendLastSentWas(manager, 500);

    EXPECT_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest()).Times(0);

    manager.Process();
}

// ============================================================================
// F013 팬텀 퍼블리셔 가드
//
// StartService/StopService 엔드포인트가 "이 명령을 보낸 참가자가 진짜
// lifecycle_mgr 인가" 를 이걸로 판정한다. 좀비 DDS 참가자나 유출된 테스트
// 픽스처가 raw 토픽에 직접 쓰는 것을 여기서 떨군다 — MANUAL 서비스가
// 운영자가 Stop 시킨 뒤 스스로 되살아나는 것을 막는 지점이다.
//
// 실행 카운트가 0 이었다. 순수 로직이라 DDS 없이 검증할 수 있다.
// ============================================================================

namespace {
eprosima::fastdds::dds::InstanceHandle_t MakeHandle(uint8_t tag) {
    eprosima::fastdds::dds::InstanceHandle_t h;
    // 가드는 앞 12바이트(GUID prefix)만 본다. 뒷부분은 entity id 라 무시된다.
    for (int i = 0; i < 12; ++i) {
        h.value[i] = static_cast<uint8_t>(tag + i);
    }
    return h;
}
} // namespace

TEST_F(ADSWManagerTest, PhantomGuard_RefusesEverythingBeforeHandshake) {
    EXPECT_FALSE(manager.KnownLifecyclePublisherSet());
    // 신뢰 앵커가 아직 없으면 무조건 거절한다. lifecycle_mgr 는 핸드셰이크
    // 전에 Start 를 보내지 않으므로 정상 트래픽을 떨구지 않는다.
    EXPECT_FALSE(manager.IsKnownLifecyclePublisher(MakeHandle(0x10)));
}

TEST_F(ADSWManagerTest, PhantomGuard_TrustsTheAnchoredPrefix) {
    manager.SetKnownLifecyclePublisher(MakeHandle(0x10));

    EXPECT_TRUE(manager.KnownLifecyclePublisherSet());
    EXPECT_TRUE(manager.IsKnownLifecyclePublisher(MakeHandle(0x10)));
}

TEST_F(ADSWManagerTest, PhantomGuard_RefusesAnUnknownPrefix) {
    manager.SetKnownLifecyclePublisher(MakeHandle(0x10));

    EXPECT_FALSE(manager.IsKnownLifecyclePublisher(MakeHandle(0x90)))
        << "다른 참가자가 보낸 명령을 통과시켰다";
}

TEST_F(ADSWManagerTest, PhantomGuard_AnchoringTwiceIsIdempotent) {
    manager.SetKnownLifecyclePublisher(MakeHandle(0x10));
    EXPECT_NO_THROW(manager.SetKnownLifecyclePublisher(MakeHandle(0x10)));

    EXPECT_EQ(manager.lifecycle_trusted_prefixes_.size(), 1u);
    EXPECT_TRUE(manager.IsKnownLifecyclePublisher(MakeHandle(0x10)));
}

TEST_F(ADSWManagerTest, PhantomGuard_HoldsMultipleAnchors) {
    // lifecycle_mgr 가 재시작하면 새 GUID prefix 로 다시 앵커링된다.
    // 옛 앵커도 남아 있어야 재시작 직전에 발행된 메시지가 떨어지지 않는다.
    manager.SetKnownLifecyclePublisher(MakeHandle(0x10));
    manager.SetKnownLifecyclePublisher(MakeHandle(0x40));

    EXPECT_EQ(manager.lifecycle_trusted_prefixes_.size(), 2u);
    EXPECT_TRUE(manager.IsKnownLifecyclePublisher(MakeHandle(0x10)));
    EXPECT_TRUE(manager.IsKnownLifecyclePublisher(MakeHandle(0x40)));
    EXPECT_FALSE(manager.IsKnownLifecyclePublisher(MakeHandle(0x90)));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
