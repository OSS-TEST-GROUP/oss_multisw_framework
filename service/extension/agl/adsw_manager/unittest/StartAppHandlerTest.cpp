#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestPrinter.hpp"

#define private public
#include "StartAppHandler.hpp"
#undef private

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "lifecycle/StartServiceRequestPubSubTypes.hpp"
#include "../common/AppUtils.hpp"

using namespace adsw_manager::agl;
using namespace eprosima::fastdds::dds;

class StartAppHandlerTest : public ::testing::Test {
protected:
    static DomainParticipant* participant_;
    static Publisher* publisher_;
    static Subscriber* subscriber_;

    StartAppHandler test_handler;
    StartAppHandler::RequestCallback cb = [](const lifecyclemgr::StartServiceRequest&) {};

    static void SetUpTestSuite() {
        DomainParticipantQos qos;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, qos);
        ASSERT_NE(nullptr, participant_);

        publisher_  = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        ASSERT_NE(nullptr, publisher_);
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        ASSERT_NE(nullptr, subscriber_);
    }

    static void TearDownTestSuite() {
        if (participant_) {
            if (publisher_)  { participant_->delete_publisher(publisher_);  publisher_  = nullptr; }
            if (subscriber_) { participant_->delete_subscriber(subscriber_); subscriber_ = nullptr; }
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }
};

DomainParticipant* StartAppHandlerTest::participant_ = nullptr;
Publisher*        StartAppHandlerTest::publisher_   = nullptr;
Subscriber*       StartAppHandlerTest::subscriber_  = nullptr;

// ----------------------------- Initialize: success 경로 -----------------------------

TEST_F(StartAppHandlerTest, StartAppHandler_Initialize_Succeeds_WithValidArgs) {
    EXPECT_TRUE(test_handler.Initialize(participant_, publisher_, subscriber_, cb));
}

// ----------------------------- Initialize: 실패 경로들 -----------------------------

TEST_F(StartAppHandlerTest, Initialize_Fails_WhenParticipantIsNull) {
    StartAppHandler handler;
    EXPECT_FALSE(handler.Initialize(nullptr, publisher_, subscriber_, cb));
}

TEST_F(StartAppHandlerTest, Initialize_Fails_WhenPublisherIsNull) {
    StartAppHandler handler;
    EXPECT_FALSE(handler.Initialize(participant_, nullptr, subscriber_, cb));
}

TEST_F(StartAppHandlerTest, Initialize_Fails_WhenSubscriberIsNull) {
    StartAppHandler handler;
    EXPECT_FALSE(handler.Initialize(participant_, publisher_, nullptr, cb));
}

TEST_F(StartAppHandlerTest, Initialize_Fails_WhenCallbackIsEmpty) {
    StartAppHandler handler;
    StartAppHandler::RequestCallback empty_cb;  // std::function 비어 있는 상태

    EXPECT_FALSE(handler.Initialize(participant_, publisher_, subscriber_, empty_cb));
}

// ----------------------------- PublishResponse: 성공 경로 -----------------------------

TEST_F(StartAppHandlerTest, StartAppHandler_PublishResponse_Succeeds_AfterInitialize) {
    ASSERT_TRUE(test_handler.Initialize(participant_, publisher_, subscriber_, cb))
        << "Initialize failed -> writer_가 nullptr일 가능성 큼";

    lifecyclemgr::StartServiceResponse res;
    res.ret_code(lifecyclemgr::ReturnCode::SUCCESS);

    EXPECT_TRUE(test_handler.PublishResponse(res));
}

// ----------------------------- Destructor: null 포인터 브랜치 -----------------------------

TEST_F(StartAppHandlerTest, Destructor_WhenNoReaderWriterOrTopics_DoesNotCrash) {
    StartAppHandler handler;

    handler.participant_ = participant_;
    handler.publisher_   = publisher_;
    handler.subscriber_  = subscriber_;

    handler.req_topic_ = nullptr;
    handler.res_topic_ = nullptr;
    handler.reader_    = nullptr;
    handler.writer_    = nullptr;

}
