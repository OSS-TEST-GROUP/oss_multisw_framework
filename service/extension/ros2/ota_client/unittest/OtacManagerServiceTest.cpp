#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock/MockOtacStatusStore.hpp"
#include "mock/MockDdsAbstraction.hpp"

#define private public
#define protected public
#include "OtacManagerService.hpp"
#undef private
#undef protected

#include "ota_mgr/OtamClientServiceMsg.hpp"

#include <fastdds/dds/subscriber/SampleInfo.hpp>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

using namespace ros2::ota_client;
using OtaManager::SetOtaClientRegistration;
using eprosima::fastdds::dds::SampleInfo;

class OtacManagerServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<OtacStatusStore> dummy_store_{nullptr};
    OtacManagerService svc_{dummy_store_};

    MockDdsAbstraction* reg_{nullptr};
    MockDdsAbstraction* reg_resp_{nullptr};
    MockDdsAbstraction* mgr_ready_{nullptr};
    MockDdsAbstraction* upd_{nullptr};

    void SetUp() override {
        svc_.registration_dds_ = std::unique_ptr<DdsAbstraction>(reg_ = new MockDdsAbstraction());
        svc_.registration_response_dds_ = std::unique_ptr<DdsAbstraction>(reg_resp_ = new MockDdsAbstraction());
        svc_.manager_ready_dds_ = std::unique_ptr<DdsAbstraction>(mgr_ready_ = new MockDdsAbstraction());
        svc_.update_status_dds_ = std::unique_ptr<DdsAbstraction>(upd_ = new MockDdsAbstraction());
    }
};

TEST_F(OtacManagerServiceTest, Start_CallsCreateWithExpectedTopicsAndFlags) {
    EXPECT_CALL(*reg_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration",
                              /*is_writer*/true, /*is_reader*/false))
        .WillOnce(Return(true));

    EXPECT_CALL(*reg_resp_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse",
                                   /*is_writer*/false, /*is_reader*/true))
        .WillOnce(Return(true));

    EXPECT_CALL(*mgr_ready_, Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady",
                                    /*is_writer*/false, /*is_reader*/true))
        .WillOnce(Return(true));

    EXPECT_CALL(*upd_, Create("/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus",
                              /*is_writer*/false, /*is_reader*/true))
        .WillOnce(Return(true));

    EXPECT_TRUE(svc_.Start());
}

TEST_F(OtacManagerServiceTest, Start_FailsWhenRegistrationCreateFails) {
    EXPECT_CALL(*reg_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration",
                              true, false))
        .WillOnce(Return(false));
    EXPECT_CALL(*reg_resp_, Create(_, _, _)).Times(0);
    EXPECT_CALL(*mgr_ready_, Create(_, _, _)).Times(0);
    EXPECT_CALL(*upd_, Create(_, _, _)).Times(0);

    EXPECT_FALSE(svc_.Start());
}

TEST_F(OtacManagerServiceTest, Start_FailsWhenRegistrationResponseCreateFails) {
    EXPECT_CALL(*reg_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration",
                              true, false))
        .WillOnce(Return(true));
    EXPECT_CALL(*reg_resp_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse",
                                   false, true))
        .WillOnce(Return(false));
    EXPECT_CALL(*mgr_ready_, Create(_, _, _)).Times(0);
    EXPECT_CALL(*upd_, Create(_, _, _)).Times(0);

    EXPECT_FALSE(svc_.Start());
}

TEST_F(OtacManagerServiceTest, Start_FailsWhenManagerReadyCreateFails) {
    EXPECT_CALL(*reg_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration",
                              true, false))
        .WillOnce(Return(true));
    EXPECT_CALL(*reg_resp_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse",
                                   false, true))
        .WillOnce(Return(true));
    EXPECT_CALL(*mgr_ready_, Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady",
                                    false, true))
        .WillOnce(Return(false));
    EXPECT_CALL(*upd_, Create(_, _, _)).Times(0);

    EXPECT_FALSE(svc_.Start());
}

TEST_F(OtacManagerServiceTest, Start_FailsWhenUpdateStatusCreateFails) {
    EXPECT_CALL(*reg_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration",
                              true, false))
        .WillOnce(Return(true));
    EXPECT_CALL(*reg_resp_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse",
                                   false, true))
        .WillOnce(Return(true));
    EXPECT_CALL(*mgr_ready_, Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady",
                                    false, true))
        .WillOnce(Return(true));
    EXPECT_CALL(*upd_, Create("/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus",
                              false, true))
        .WillOnce(Return(false));

    EXPECT_FALSE(svc_.Start());
}

TEST_F(OtacManagerServiceTest, PublishClientRegistration_BuildsMessageAndWrites) {
    std::shared_ptr<void> captured;
    EXPECT_CALL(*reg_, Write(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

    const std::string client = "client_alpha";
    std::vector<std::string> apps = {"nav_app", "music_app", "ota_agent"};

    const bool ok = svc_.PublishClientRegistration(client, apps);
    EXPECT_TRUE(ok);

    auto reg_msg = std::static_pointer_cast<SetOtaClientRegistration>(captured);
    ASSERT_TRUE(reg_msg != nullptr);

    EXPECT_EQ(reg_msg->client_name(), client);
    ASSERT_EQ(reg_msg->app_list().size(), apps.size());
    for (size_t i = 0; i < apps.size(); ++i) {
        EXPECT_EQ(reg_msg->app_list()[i], apps[i]);
    }
}

TEST_F(OtacManagerServiceTest, PublishClientRegistration_PropagatesWriteFailure) {
    EXPECT_CALL(*reg_, Write(_)).WillOnce(Return(false));
    EXPECT_FALSE(svc_.PublishClientRegistration("c", {"a"}));
}

TEST_F(OtacManagerServiceTest, OnManagerReady_GetsAppListAndPublishesRegistration) {
    auto store = new MockOtacStatusStore();
    svc_.status_store_ = std::shared_ptr<OtacStatusStore>(
        store, [](OtacStatusStore* p){ delete static_cast<MockOtacStatusStore*>(p); });

    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    std::shared_ptr<void> captured;
    EXPECT_CALL(*reg_, Write(_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

    OtaManager::NotifyOtaManagerReady msg;
    svc_.OnManagerReady(msg);

    auto reg_msg = std::static_pointer_cast<OtaManager::SetOtaClientRegistration>(captured);
    ASSERT_TRUE(reg_msg);
    EXPECT_EQ(reg_msg->client_name(), "ros2_ota_client");
    ASSERT_EQ(reg_msg->app_list().size(), apps.size());
    for (size_t i = 0; i < apps.size(); ++i) {
        EXPECT_EQ(reg_msg->app_list()[i], apps[i]);
    }
}

TEST_F(OtacManagerServiceTest, OnManagerReady_WhenPublishFails_NoSuccessLog) {
    auto store = new MockOtacStatusStore();
    svc_.status_store_ = std::shared_ptr<OtacStatusStore>(
        store, [](OtacStatusStore* p){ delete static_cast<MockOtacStatusStore*>(p); });

    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    EXPECT_CALL(*reg_, Write(_))
        .Times(1)
        .WillOnce(Return(false));

    testing::internal::CaptureStdout();
    OtaManager::NotifyOtaManagerReady msg;
    svc_.OnManagerReady(msg);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(out.find("Successfully Call Publish."), std::string::npos);
}

TEST_F(OtacManagerServiceTest, OnAppUpdateStatus_DelegatesToStoreUpdateStatus) {
    auto store = new MockOtacStatusStore();
    svc_.status_store_ = std::shared_ptr<OtacStatusStore>(
        store, [](OtacStatusStore* p){ delete static_cast<MockOtacStatusStore*>(p); });

    OtaManager::NotifyAppUpdateStatus msg;
    msg.app_name("nav_app");

    using StatusT = MockOtacStatusStore::StatusType;
    EXPECT_CALL(*store, UpdateStatus("nav_app", ::testing::_)).Times(1);

    svc_.OnAppUpdateStatus(msg);
}

TEST_F(OtacManagerServiceTest, OnClientRegistrationResponse_PrintsClientIdAndResult) {
    OtaManager::SetOtaClientRegistrationResponse resp;
    resp.client_id(1234);

    testing::internal::CaptureStdout();
    svc_.OnClientRegistrationResponse(resp);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("Client id: 1234"), std::string::npos);
    EXPECT_NE(out.find("result:"), std::string::npos);
}

class OtacManagerServiceInitRealDdsTest : public ::testing::Test {
protected:
    OtacManagerService svc_{nullptr};
};

TEST_F(OtacManagerServiceInitRealDdsTest, Initialize_CreatesEntitiesAndDdsImpls) {
    ASSERT_TRUE(svc_.Initialize());

    EXPECT_NE(svc_.participant_, nullptr);
    EXPECT_NE(svc_.publisher_, nullptr);
    EXPECT_NE(svc_.subscriber_, nullptr);
    EXPECT_NE(svc_.registration_dds_, nullptr);
    EXPECT_NE(svc_.registration_response_dds_, nullptr);
    EXPECT_NE(svc_.manager_ready_dds_, nullptr);
    EXPECT_NE(svc_.update_status_dds_, nullptr);

    EXPECT_NO_THROW(svc_.Stop());
}

TEST(OtacManagerServiceStopTest, Stop_DoesNothingWhenParticipantIsNull) {
    OtacManagerService svc{nullptr};
    EXPECT_NO_THROW(svc.Stop());
}