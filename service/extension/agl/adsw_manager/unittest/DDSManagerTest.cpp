#include <gtest/gtest.h>

#define private public
#include "DDSManager.hpp"
#undef private

#include "mock/MockNotifyAppStateWriter.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include "agl/NotifyAppStatePubSubTypes.hpp"

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

// ----------------------------- Initialize -----------------------------

TEST_F(DDSManagerTest_Uninit, DDSManager_Initialize) {
    EXPECT_TRUE(uut->Initialize(participant_));
}

class DDSManagerTest_Init : public DDSManagerSuite {
    protected:
        DDSManager* uut_init = nullptr;

        void SetUp() override {
            uut_init = new DDSManager;
            uut_init->Initialize(participant_);
        }

        void TearDown() override {
            delete uut_init;
        }
};

// ----------------------------- PublishNotifyAppState -----------------------------

TEST_F(DDSManagerTest_Init, DDSManager_PublishNotify) {

    auto mock = std::make_unique<StrictMock<MockNotifyAppStateWriter>>();
    EXPECT_CALL(*mock, Publish(_)).Times(1).WillOnce(Return(true));
    uut_init->InjectNotifyWriterForTest(std::move(mock));

    adsw_manager_agl::NotifyAppState msg;

    msg.app_name() = "test";
    msg.app_state() = adsw_manager_agl::AppState::DETACHED;

    uut_init->PublishNotifyAppState(msg);

}

// ----------------------------- SetOnGetAppStateRequest -----------------------------

TEST_F(DDSManagerTest_Init, SetOnGetAppStateRequest_StoresAndInvokes) {

    MockFunction<void(const adsw_manager_agl::GetAppStateRequest&)> cb;

    uut_init->SetOnGetAppStateRequest(cb.AsStdFunction());

    EXPECT_TRUE(static_cast<bool>(uut_init->on_get_app_state_request_));

    adsw_manager_agl::GetAppStateRequest req;
    EXPECT_CALL(cb, Call(_)).Times(1);
    uut_init->on_get_app_state_request_(req);

}

// ----------------------------- SetOnStartAppRequest -----------------------------

TEST_F(DDSManagerTest_Init, SetOnStartAppRequest_StoresAndInvokes) {

    MockFunction<void(const adsw_manager_agl::StartAppRequest&)> cb;

    uut_init->SetOnStartAppRequest(cb.AsStdFunction());

    EXPECT_TRUE(static_cast<bool>(uut_init->on_start_app_request_));

    adsw_manager_agl::StartAppRequest req;
    EXPECT_CALL(cb, Call(_)).Times(1);
    uut_init->on_start_app_request_(req);

}

// ----------------------------- SetOnStopAppRequest -----------------------------

TEST_F(DDSManagerTest_Init, SetOnStopAppRequest_StoresAndInvokes) {

    MockFunction<void(const adsw_manager_agl::StopAppRequest&)> cb;

    uut_init->SetOnStopAppRequest(cb.AsStdFunction());

    EXPECT_TRUE(static_cast<bool>(uut_init->on_stop_app_request_));

    adsw_manager_agl::StopAppRequest req;
    EXPECT_CALL(cb, Call(_)).Times(1);
    uut_init->on_stop_app_request_(req);

}