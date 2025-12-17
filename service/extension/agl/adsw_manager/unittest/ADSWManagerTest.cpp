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
#include "mock/MockDDSManager.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace testing;
using namespace adsw_manager::agl;

class ADSWManagerTestFixture : public ::testing::Test {
protected:
    // ADSWManager manager;

    eprosima::fastdds::dds::DomainParticipant* participant = nullptr;
    
    std::shared_ptr<NiceMock<MockServiceRegistry>> mock_registry;
    std::shared_ptr<NiceMock<MockAppController>> mock_app_controller;
    std::shared_ptr<NiceMock<MockServiceController>> mock_controller;
    std::shared_ptr<NiceMock<MockDDSManager>> mock_ddsmanager;
    std::shared_ptr<NiceMock<MockMessageTransmitter>> mock_transmitter;
    std::shared_ptr<NiceMock<MockServiceMonitor>> mock_monitor;

    void SetUp() override {

        eprosima::fastdds::dds::DomainParticipantQos qos;
        qos.name("adsw_manager_unittest_participant");
        participant = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(0, qos);
        ASSERT_NE(participant, nullptr);

        mock_registry = std::make_shared<NiceMock<MockServiceRegistry>>();
        mock_app_controller = std::make_shared<NiceMock<MockAppController>>();
        mock_controller = std::make_shared<NiceMock<MockServiceController>>(mock_app_controller.get());
        mock_ddsmanager = std::make_shared<NiceMock<MockDDSManager>>();
        mock_transmitter = std::make_shared<NiceMock<MockMessageTransmitter>>(mock_registry, mock_controller, mock_ddsmanager);
        mock_monitor = std::make_shared<NiceMock<MockServiceMonitor>>();

        // ON_CALL(*mock_registry, Initialize()).WillByDefault(Return(true));
        // ON_CALL(*mock_controller, Initialize()).WillByDefault(Return(true));
        // ON_CALL(*mock_transmitter, Initialize(_)).WillByDefault(Return(true));
        // ON_CALL(*mock_monitor, Initialize(_,_,_)).WillByDefault(Return(true));
    }

    void TearDown() override {
        if (participant) {
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
                ->delete_participant(participant);
            participant = nullptr;
        }
    }
};

// ----------------------------- Initialize -----------------------------

TEST_F(ADSWManagerTestFixture, Initialize_NullParticipant) {
    EXPECT_CALL(*mock_registry, Initialize()).Times(0);
    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);
    
    ADSWManager manager(nullptr, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);
    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_NullRegistry) {
    EXPECT_CALL(*mock_registry, Initialize()).Times(0);
    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    ADSWManager manager(participant, nullptr, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);
    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_NullAppController) {
    EXPECT_CALL(*mock_registry, Initialize()).Times(0);
    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    ADSWManager manager(participant, mock_registry, nullptr, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);
    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_NullServiceController) {
    EXPECT_CALL(*mock_registry, Initialize()).Times(0);
    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        nullptr, mock_ddsmanager, mock_transmitter, mock_monitor);
    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_NullDDSManager) {
    EXPECT_CALL(*mock_registry, Initialize()).Times(0);
    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, nullptr, mock_transmitter, mock_monitor);
    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_NullTransmitter) {
    EXPECT_CALL(*mock_registry, Initialize()).Times(0);
    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, nullptr, mock_monitor);
    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_NullMonitor) {
    EXPECT_CALL(*mock_registry, Initialize()).Times(0);
    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, nullptr);
    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_Success_WiresEverythingInOrder) {
    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);

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

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);

    EXPECT_CALL(*mock_registry, Initialize()).WillOnce(Return(false));

    EXPECT_CALL(*mock_controller, Initialize()).Times(0);
    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_Fails_WhenServiceControllerInitFails) {

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);

    InSequence seq;
    EXPECT_CALL(*mock_registry, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_controller, Initialize()).WillOnce(Return(false));

    EXPECT_CALL(*mock_transmitter, Initialize(_)).Times(0);
    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_Fails_WhenMessageTransmitterInitFails) {

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);

    InSequence seq;
    EXPECT_CALL(*mock_registry, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_controller, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, Initialize(_)).WillOnce(Return(false));

    EXPECT_CALL(*mock_monitor, Initialize(_,_,_)).Times(0);

    EXPECT_FALSE(manager.Initialize());
}

TEST_F(ADSWManagerTestFixture, Initialize_Fails_WhenServiceMonitorInitFails) {

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);

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

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);

    InSequence seq;
    EXPECT_CALL(*mock_transmitter, PublishAppList()).Times(1);
    EXPECT_CALL(*mock_monitor, StartMonitoring()).Times(1);

    EXPECT_TRUE(manager.StartService());
}

// ----------------------------- StopService -----------------------------

TEST_F(ADSWManagerTestFixture, StopService_CallsStopMonitoring) {

    ADSWManager manager(participant, mock_registry, mock_app_controller, 
        mock_controller, mock_ddsmanager, mock_transmitter, mock_monitor);

    EXPECT_CALL(*mock_monitor, StopMonitoring()).Times(1);
    EXPECT_TRUE(manager.StopService());
}
