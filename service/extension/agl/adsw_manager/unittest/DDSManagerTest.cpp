#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestPrinter.hpp"

#define private public
#include "DDSManager.hpp"
#undef private

#include "mock/MockNotifyAppStateWriter.hpp"
#include "mock/MockUpdateAppListHandler.hpp"
#include "mock/MockStartAppHandler.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include "lifecycle/NotifyServiceStatePubSubTypes.hpp"
#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"
#include "lifecycle/StartServiceRequestPubSubTypes.hpp"

#include "../common/AppUtils.hpp"

using namespace eprosima::fastdds::dds;
using namespace adsw_manager::agl;
using namespace testing;

class DDSManagerSuite : public ::testing::Test {
protected:
    static DomainParticipant* participant_;
    
    static void SetUpTestSuite() {
        DomainParticipantQos qos;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, qos);
        ASSERT_NE(participant_, nullptr);
    }

    static void TearDownTestSuite() {
        if (participant_) {
            participant_->delete_contained_entities();
            EXPECT_EQ(DomainParticipantFactory::get_instance()->delete_participant(participant_), RETCODE_OK);
            participant_ = nullptr;
        }
    }
};

DomainParticipant* DDSManagerSuite::participant_ = nullptr;

class DDSManagerTest_Uninit : public DDSManagerSuite {
protected:
    DDSManager* uut = nullptr;

    void SetUp() override {
        uut = new DDSManager;
    }

    void TearDown() override {
        delete uut;
    }
};

TEST_F(DDSManagerTest_Uninit, Initialize_UpdateListHandlerCallback_InvokesOnUpdateCallback) {
    using lifecyclemgr::UpdateServiceListResponse;

    auto mock_handler = std::make_unique<NiceMock<MockUpdateAppListHandler>>();
    std::function<void(const UpdateServiceListResponse&)> saved_cb;

    EXPECT_CALL(*mock_handler, Initialize(_, _, _, _))
        .WillOnce(DoAll(
            SaveArg<3>(&saved_cb),
            Return(true)
        ));

    uut->InjectUpdateAppListHandlerForTest(std::move(mock_handler));

    MockFunction<void(const UpdateServiceListResponse&)> user_cb;
    uut->SetOnUpdateListResponse(user_cb.AsStdFunction());

    ASSERT_TRUE(uut->Initialize(participant_));
    ASSERT_TRUE(static_cast<bool>(saved_cb));

    UpdateServiceListResponse res;

    EXPECT_CALL(user_cb, Call(_)).Times(1);
    saved_cb(res);
}

TEST_F(DDSManagerTest_Uninit, Destructor_WithNoParticipant_NoCrash) {
    SUCCEED();
}

TEST_F(DDSManagerTest_Uninit, PublishNotifyAppState_NoWriter_DoesNothing) {
    lifecyclemgr::NotifyServiceState msg;
    msg.service_list().push_back("dummy");
    msg.service_state_list().push_back(lifecyclemgr::ServiceState::NOT_RUNNING);

    uut->PublishNotifyAppState(msg);
}

TEST_F(DDSManagerTest_Uninit, PublishUpdateAppList_NoHandler_DoesNothing) {
    lifecyclemgr::UpdateServiceListRequest req;
    uut->PublishUpdateAppList(req);
}

TEST_F(DDSManagerTest_Uninit, Initialize_UsesInjectedNotifyWriter_Success) {
    auto mock_writer = std::make_unique<StrictMock<MockNotifyAppStateWriter>>();

    EXPECT_CALL(*mock_writer, Initialize(_, _))
        .Times(1)
        .WillOnce(Return(true));

    uut->InjectNotifyWriterForTest(std::move(mock_writer));

    EXPECT_TRUE(uut->Initialize(participant_));
}

TEST_F(DDSManagerTest_Uninit, Initialize_FailsWhenNotifyWriterInitializeFails) {
    auto mock_writer = std::make_unique<StrictMock<MockNotifyAppStateWriter>>();

    EXPECT_CALL(*mock_writer, Initialize(_, _))
        .Times(1)
        .WillOnce(Return(false));   // 실패 분기

    uut->InjectNotifyWriterForTest(std::move(mock_writer));

    EXPECT_FALSE(uut->Initialize(participant_));
}

class DDSManagerTest_Init : public DDSManagerSuite {
protected:
    DDSManager* uut_init = nullptr;

    void SetUp() override {
        uut_init = new DDSManager;
        ASSERT_TRUE(uut_init->Initialize(participant_));
    }

    void TearDown() override {
        delete uut_init;
    }
};

TEST_F(DDSManagerTest_Init, PublishNotifyAppState_UsesWriter_WhenInitialized) {
    // NotifyAppStateWriter를 Mo ck 으로 교체해서 Publish 호출 여부 검증
    auto mock_writer = std::make_unique<StrictMock<MockNotifyAppStateWriter>>();
    EXPECT_CALL(*mock_writer, Publish(_))
        .Times(1)
        .WillOnce(Return(true));

    uut_init->InjectNotifyWriterForTest(std::move(mock_writer));

    lifecyclemgr::NotifyServiceState msg;
    msg.service_list().push_back("test");
    msg.service_state_list().push_back(lifecyclemgr::ServiceState::NOT_RUNNING);

    uut_init->PublishNotifyAppState(msg);
}

TEST_F(DDSManagerTest_Init, PublishUpdateAppList_UsesHandler_WhenInjected) {
    auto mock_handler = std::make_unique<StrictMock<MockUpdateAppListHandler>>();

    EXPECT_CALL(*mock_handler, Publish(_))
        .Times(1);

    uut_init->InjectUpdateAppListHandlerForTest(std::move(mock_handler));

    lifecyclemgr::UpdateServiceListRequest req;
    lifecyclemgr::Service svc;
    svc.name("svc");
    req.service_list().push_back(svc);

    uut_init->PublishUpdateAppList(req);
}

// SetOnGetAppStateRequest
TEST_F(DDSManagerTest_Init, SetOnGetAppStateRequest_StoresAndInvokes) {
    MockFunction<void(const lifecyclemgr::GetServiceStateRequest&)> cb;

    uut_init->SetOnGetAppStateRequest(cb.AsStdFunction());
    EXPECT_TRUE(static_cast<bool>(uut_init->on_get_app_state_request_));

    lifecyclemgr::GetServiceStateRequest req;
    EXPECT_CALL(cb, Call(_)).Times(1);
    uut_init->on_get_app_state_request_(req);
}

// SetOnStartAppRequest
TEST_F(DDSManagerTest_Init, SetOnStartAppRequest_StoresAndInvokes) {
    MockFunction<void(const lifecyclemgr::StartServiceRequest&)> cb;

    uut_init->SetOnStartAppRequest(cb.AsStdFunction());
    EXPECT_TRUE(static_cast<bool>(uut_init->on_start_app_request_));

    lifecyclemgr::StartServiceRequest req;
    EXPECT_CALL(cb, Call(_)).Times(1);
    uut_init->on_start_app_request_(req);
}

// SetOnStopAppRequest
TEST_F(DDSManagerTest_Init, SetOnStopAppRequest_StoresAndInvokes) {
    MockFunction<void(const lifecyclemgr::StopServiceRequest&)> cb;

    uut_init->SetOnStopAppRequest(cb.AsStdFunction());
    EXPECT_TRUE(static_cast<bool>(uut_init->on_stop_app_request_));

    lifecyclemgr::StopServiceRequest req;
    EXPECT_CALL(cb, Call(_)).Times(1);
    uut_init->on_stop_app_request_(req);
}

// SetOnUpdateListResponse
TEST_F(DDSManagerTest_Init, SetOnUpdateListResponse_StoresAndInvokes) {
    MockFunction<void(const lifecyclemgr::UpdateServiceListResponse&)> cb;

    uut_init->SetOnUpdateListResponse(cb.AsStdFunction());
    EXPECT_TRUE(static_cast<bool>(uut_init->on_update_list_response_));

    lifecyclemgr::UpdateServiceListResponse res;
    EXPECT_CALL(cb, Call(_)).Times(1);
    uut_init->on_update_list_response_(res);
}

TEST_F(DDSManagerTest_Init, GetHandlers_ReturnNonNullAfterInitialize) {
    EXPECT_NE(uut_init->GetAppStateHandler(), nullptr);
    EXPECT_NE(uut_init->GetStartAppHandler(), nullptr);
    EXPECT_NE(uut_init->GetStopAppHandler(), nullptr);
    EXPECT_NE(uut_init->GetUpdateAppListHandler(), nullptr);
}
