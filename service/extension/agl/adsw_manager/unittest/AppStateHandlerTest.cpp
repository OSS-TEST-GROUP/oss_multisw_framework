#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "AppStateHandler.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "agl/GetAppStateResponsePubSubTypes.hpp"
#include "agl/GetAppStateRequestPubSubTypes.hpp"

#include "../common/AppUtils.hpp"

using namespace adsw_manager::agl;
using namespace eprosima::fastdds::dds;

class AppStateHandlerTest : public ::testing::Test {
protected:
    static DomainParticipant* participant_;
    static Publisher* publisher_;
    static Subscriber* subscriber_;

    AppStateHandler test_handler;
    AppStateHandler::RequestCallback cb = [](const adsw_manager_agl::GetAppStateRequest&){};

    static void SetUpTestSuite() {
        DomainParticipantQos qos;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, qos);
        ASSERT_NE(nullptr, participant_);

        publisher_   = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        ASSERT_NE(nullptr, publisher_);
        subscriber_  = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        ASSERT_NE(nullptr, subscriber_);
    }

    static void TearDownTestSuite() {
        if (participant_) {
            if (publisher_) { participant_->delete_publisher(publisher_); publisher_ = nullptr; }
            if (subscriber_) { participant_->delete_subscriber(subscriber_); subscriber_ = nullptr; }
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }
};

DomainParticipant* AppStateHandlerTest::participant_ = nullptr;
Publisher* AppStateHandlerTest::publisher_ = nullptr;
Subscriber* AppStateHandlerTest::subscriber_ = nullptr;

// ----------------------------- Initialize -----------------------------

TEST_F(AppStateHandlerTest, AppStateHandler_Initialize) {

    EXPECT_TRUE(test_handler.Initialize(participant_, publisher_, subscriber_, cb));

}

// ----------------------------- PublishResponse -----------------------------

TEST_F(AppStateHandlerTest, AppStateHandler_Publish) {

    ASSERT_TRUE(test_handler.Initialize(participant_, publisher_, subscriber_, cb))
        << "Initialize failed -> writer_가 nullptr일 가능성 큼";

    adsw_manager_agl::GetAppStateResponse res;
    res.app_list().push_back("test");
    res.app_state_list().push_back(adsw_manager_agl::AppState::RESUMED);

    EXPECT_TRUE(test_handler.PublishResponse(res));

}