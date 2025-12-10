#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestPrinter.hpp"

#define private   public
#define protected public
#include "ADSWManager.hpp"
#undef protected
#undef private

#include "mock/MockServiceRegistry.hpp"
#include "mock/MockServiceController.hpp"
#include "mock/MockAppController.hpp"
#include "mock/MockMessageTransmitter.hpp"
#include "mock/MockServiceMonitor.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace testing;
using namespace adsw_manager::agl;

class ADSWManagerTestFixture : public ::testing::Test {
protected:
    ADSWManager manager;

    std::shared_ptr<NiceMock<MockServiceRegistry>>      mock_registry;
    std::shared_ptr<NiceMock<MockAppController>>        mock_app_controller;
    std::shared_ptr<NiceMock<MockServiceController>>    mock_controller;
    std::shared_ptr<NiceMock<MockMessageTransmitter>>   mock_transmitter;
    std::shared_ptr<NiceMock<MockServiceMonitor>>       mock_monitor;

    void SetUp() override {
        mock_registry       = std::make_shared<NiceMock<MockServiceRegistry>>();
        mock_app_controller = std::make_shared<NiceMock<MockAppController>>();
        mock_controller     = std::make_shared<NiceMock<MockServiceController>>(mock_app_controller.get());
        mock_transmitter    = std::make_shared<NiceMock<MockMessageTransmitter>>();
        mock_monitor        = std::make_shared<NiceMock<MockServiceMonitor>>();

        // 의존성 직접 주입 (ADSWManager가 nullptr이면 내부에서 실객체 생성하므로, 테스트는 미리 주입)
        manager.service_registry_   = mock_registry;
        manager.app_controller_     = mock_app_controller;
        manager.service_controller_ = mock_controller;
        manager.message_transmitter_= mock_transmitter;
        manager.service_monitor_    = mock_monitor;
        manager.participant_        = nullptr; // Initialize에서 생성됨

        // 기본 동작: 정상 경로는 NiceMock 기본으로 no-op / 기본값 리턴
        ON_CALL(*mock_registry, Initialize()).WillByDefault(Return(true));
        ON_CALL(*mock_controller, Initialize()).WillByDefault(Return(true));
        ON_CALL(*mock_transmitter, Initialize(_)).WillByDefault(Return(true));
        ON_CALL(*mock_monitor, Initialize(_,_,_)).WillByDefault(Return(true));
    }

    void TearDown() override {
        if (manager.participant_) {
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
                ->delete_participant(manager.participant_);
            manager.participant_ = nullptr;
        }
    }
};

// ----------------------------- Initialize -----------------------------

TEST_F(ADSWManagerTestFixture, Initialize_Success_WiresEverythingInOrder) {
    InSequence seq; // 호출 순서 강제

    EXPECT_CALL(*mock_registry, Initialize())
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_controller, Initialize())
        .Times(1)
        .WillOnce(Return(true));

    // participant가 동일하게 두 군데로 전달되는지 캡처
    eprosima::fastdds::dds::DomainParticipant* p_to_tx  = nullptr;
    eprosima::fastdds::dds::DomainParticipant* p_to_mon = nullptr;

    EXPECT_CALL(*mock_transmitter, Initialize(_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&p_to_tx), Return(true)));

    std::shared_ptr<ServiceRegistry> passed_reg;
    std::shared_ptr<MessageTransmitter> passed_tx;

    EXPECT_CALL(*mock_monitor, Initialize(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&p_to_mon),
                        SaveArg<1>(&passed_reg),
                        SaveArg<2>(&passed_tx),
                        Return(true)));

    EXPECT_TRUE(manager.Initialize());

    // 생성/전달값 검증
    ASSERT_NE(manager.participant_, nullptr);
    ASSERT_NE(p_to_tx, nullptr);
    ASSERT_NE(p_to_mon, nullptr);
    EXPECT_EQ(p_to_tx,  manager.participant_);
    EXPECT_EQ(p_to_mon, manager.participant_);

    EXPECT_EQ(passed_reg.get(), mock_registry.get());
    EXPECT_EQ(passed_tx.get(),  mock_transmitter.get());
}

TEST_F(ADSWManagerTestFixture, Initialize_Fails_WhenServiceRegistryInitFails) {
    EXPECT_CALL(*mock_registry, Initialize()).WillOnce(Return(false));

    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_Fails_WhenServiceControllerInitFails) {
    InSequence seq;
    EXPECT_CALL(*mock_registry, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_controller, Initialize()).WillOnce(Return(false));

    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_Fails_WhenMessageTransmitterInitFails) {
    InSequence seq;
    EXPECT_CALL(*mock_registry, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_controller, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, Initialize(_)).WillOnce(Return(false));

    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_Fails_WhenServiceMonitorInitFails) {
    InSequence seq;
    EXPECT_CALL(*mock_registry, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_controller, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).WillOnce(Return(false));

    EXPECT_FALSE(manager.Initialize());
}

// ----------------------------- StartService -----------------------------

TEST_F(ADSWManagerTestFixture, StartService_PublishesAppList_Then_StartsMonitoring) {
    // StartService는 PublishAppList() → StartMonitoring() 순
    InSequence seq;
    EXPECT_CALL(*mock_transmitter, PublishAppList()).Times(1);
    EXPECT_CALL(*mock_monitor, StartMonitoring()).Times(1);

    EXPECT_TRUE(manager.StartService());
}

// ----------------------------- StopService -----------------------------

TEST_F(ADSWManagerTestFixture, StopService_CallsStopMonitoring) {
    EXPECT_CALL(*mock_monitor, StopMonitoring()).Times(1);
    EXPECT_TRUE(manager.StopService());
}
