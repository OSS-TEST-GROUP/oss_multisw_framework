#include "gtest/gtest.h"
#include "dds_transport.h"
#include "ThreadSafeQueue.h"
#include "ObjectPool.h"
#include "Types.h"
#include "VissRequestPubSubTypes.hpp"
#include "Logger.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <thread>
#include <chrono>

using namespace eprosima::fastdds::dds;

const std::string VISS_DDS_REQUEST_TOPIC("/VIN/Vehicle");

class ResponseListener : public DataReaderListener {
public:
    std::atomic<bool> received{false};
    std::string payload;
    std::mutex mtx;
    std::condition_variable cv;

    void on_data_available(DataReader* reader) override {
        SampleInfo info;
        VissRequestTopic data;
        if (reader->take_next_sample(&data, &info) == RETCODE_OK && info.valid_data) {
            std::lock_guard<std::mutex> lock(mtx);
            payload = data.payload();
            received = true;
            cv.notify_one();
        }
    }
};

class DdsTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        request_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
        auto request_pool = std::make_shared<ObjectPool<RequestData>>();
        
        transport = std::make_unique<DDSTransport>(request_pool, request_queue);
        transport->start();

        // --- Mock DDS Participant Setup ---
        eprosima::fastdds::dds::DomainParticipantQos pqos;
        pqos.name("dds_unit_test");
        participant = DomainParticipantFactory::get_instance()->create_participant(7, pqos, nullptr, eprosima::fastdds::dds::StatusMask::none());
        ASSERT_NE(participant, nullptr);

        request_type.register_type(participant);

        // --- Mock DDS Publisher for sending requests ---
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        ASSERT_NE(publisher, nullptr);
        Topic* request_topic = participant->create_topic(VISS_DDS_REQUEST_TOPIC, request_type.get_type_name(), TOPIC_QOS_DEFAULT);
        ASSERT_NE(request_topic, nullptr);
        eprosima::fastdds::dds::DataWriterQos writer_qos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
        writer_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        request_writer = publisher->create_datawriter(request_topic, writer_qos, nullptr);
        ASSERT_NE(request_writer, nullptr);

        // --- Mock DDS Subscriber for receiving responses ---
        subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        ASSERT_NE(subscriber, nullptr);
        response_topic = participant->create_topic(RESPONSE_TOPIC, request_type.get_type_name(), TOPIC_QOS_DEFAULT);
        ASSERT_NE(response_topic, nullptr);
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        response_reader = subscriber->create_datareader(response_topic, reader_qos, &response_listener);
        ASSERT_NE(response_reader, nullptr);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Allow discovery
    }

    void TearDown() override {
        if (participant != nullptr) {
            if (publisher != nullptr) {
                publisher->delete_datawriter(request_writer);
                participant->delete_publisher(publisher);
            }
            if (subscriber != nullptr) {
                subscriber->delete_datareader(response_reader);
                 participant->delete_subscriber(subscriber);
            }
            participant->delete_topic(request_writer->get_topic());
            participant->delete_topic(response_topic);
            DomainParticipantFactory::get_instance()->delete_participant(participant);
        }
        transport->stop();
    }

    std::unique_ptr<ITransport> transport;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> request_queue;

    // DDS Test entities
    DomainParticipant* participant = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
    DataWriter* request_writer = nullptr;
    DataReader* response_reader = nullptr;
    Topic* response_topic = nullptr;
    TypeSupport request_type{new VissRequestTopicPubSubType()};
    ResponseListener response_listener;
    const std::string RESPONSE_TOPIC = "/test/response";

    std::string CreatePayload(const std::string& viss_request) {
        rapidjson::Document payload_doc;
        payload_doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = payload_doc.GetAllocator();
        payload_doc.AddMember("topic", rapidjson::Value(RESPONSE_TOPIC.c_str(), allocator), allocator);
        payload_doc.AddMember("request", rapidjson::Value(viss_request.c_str(), allocator), allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer_json(buffer);
        payload_doc.Accept(writer_json);
        return buffer.GetString();
    }
};

TEST_F(DdsTransportTest, StartAndStop) {
    ASSERT_NO_THROW(transport->start());
    // The stop is called in TearDown, so if we reach here, it's a pass.
    SUCCEED();
}

TEST_F(DdsTransportTest, GetRequestIsQueued) {
    // 1. Prepare the inner VISS request JSON
    std::string viss_request = R"({
        "action": "get",
        "path": "Vehicle.Speed",
        "requestId": "dds-test-get-req-1"
    })";

    // 2. Prepare and publish
    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    // 3. Wait and verify that the request is queued
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-get-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "get");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Speed");

    // 4. Simulate a success response from the ServiceManager
    auto response_doc = std::make_shared<PooledResponseMap>();
    response_doc->SetObject();
    rapidjson::Document::AllocatorType& allocator = response_doc->GetAllocator();
    response_doc->AddMember("requestId", "dds-test-get-req-1", allocator);
    response_doc->AddMember("action", "get", allocator);
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", "120", allocator);
    dp_val.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    data_val.AddMember("dp", dp_val, allocator);
    response_doc->AddMember("data", data_val, allocator);
    response_doc->AddMember("ts", "2024-01-01T00:00:00Z", allocator);

    received_request->responseChan->push(response_doc);

    // 5. Wait for the response to be published back
    std::unique_lock<std::mutex> lock(response_listener.mtx);
    ASSERT_TRUE(response_listener.cv.wait_for(lock, std::chrono::seconds(1), [&]{ return response_listener.received.load(); }));
    
    // 6. Verify the response payload
    rapidjson::Document received_payload;
    received_payload.Parse(response_listener.payload.c_str());
    ASSERT_FALSE(received_payload.HasParseError());
    ASSERT_TRUE(received_payload.HasMember("data"));
    const auto& response_data = received_payload["data"];
    ASSERT_TRUE(response_data.HasMember("dp"));
}

TEST_F(DdsTransportTest, SetRequestIsQueued) {
    std::string viss_request = R"({
        "action": "set",
        "path": "Vehicle.Speed",
        "value": "88",
        "requestId": "dds-test-set-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-set-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "set");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    ASSERT_STREQ(doc["value"].GetString(), "88");

    // Simulate a success response
    auto response_doc = std::make_shared<PooledResponseMap>();
    response_doc->SetObject();
    rapidjson::Document::AllocatorType& allocator = response_doc->GetAllocator();
    response_doc->AddMember("requestId", "dds-test-set-req-1", allocator);
    response_doc->AddMember("action", "set", allocator);
    response_doc->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    received_request->responseChan->push(response_doc);

    // Wait for the response
    std::unique_lock<std::mutex> lock(response_listener.mtx);
    ASSERT_TRUE(response_listener.cv.wait_for(lock, std::chrono::seconds(1), [&]{ return response_listener.received.load(); }));

    // Verify the response
    rapidjson::Document received_payload;
    received_payload.Parse(response_listener.payload.c_str());
    ASSERT_FALSE(received_payload.HasParseError());
    ASSERT_FALSE(received_payload.HasMember("error"));
}

TEST_F(DdsTransportTest, SubscribeRequestIsQueued) {
    std::string viss_request = R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "requestId": "dds-test-sub-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-sub-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "subscribe");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
}

TEST_F(DdsTransportTest, UnsubscribeRequestIsQueued) {
    std::string viss_request = R"({
        "action": "unsubscribe",
        "subscriptionId": "sub-id-to-remove",
        "requestId": "dds-test-unsub-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-unsub-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "unsubscribe");
    ASSERT_STREQ(doc["subscriptionId"].GetString(), "sub-id-to-remove");
}

TEST_F(DdsTransportTest, GetRequestWithPathFilter) {
    std::string viss_request = R"({
        "action": "get",
        "path": "Vehicle.Cabin",
        "filter": {"variant": "paths", "parameter": ["Sunroof.Position", "Light.IsDomeOn"]},
        "requestId": "dds-test-get-path-filter-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-get-path-filter-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "get");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Cabin");
    ASSERT_TRUE(doc.HasMember("filter"));
}

TEST_F(DdsTransportTest, SubscribeWithTimeBasedFilter) {
    std::string viss_request = R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "filter": {"variant": "timebased", "parameter": {"period": "1000"}},
        "requestId": "dds-test-sub-time-filter-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-sub-time-filter-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "subscribe");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    ASSERT_TRUE(doc.HasMember("filter"));
}

TEST_F(DdsTransportTest, GetRequestWithMetadataFilter) {
    std::string viss_request = R"({
        "action": "get",
        "path": "Vehicle.Speed",
        "filter": {"variant": "metadata", "parameter": "1"},
        "requestId": "dds-test-get-metadata-filter-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-get-metadata-filter-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "get");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    ASSERT_TRUE(doc.HasMember("filter"));
}

TEST_F(DdsTransportTest, GetRequestWithHistoryFilter) {
    std::string viss_request = R"({
        "action": "get",
        "path": "Vehicle.Speed",
        "filter": [{"variant": "history", "parameter": "PT1M"}],
        "requestId": "dds-test-get-history-filter-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-get-history-filter-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "get");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    ASSERT_TRUE(doc.HasMember("filter"));
}

TEST_F(DdsTransportTest, GetRequestWithCombinedFilters) {
    std::string viss_request = R"({
        "action": "get",
        "path": "Vehicle.Cabin",
        "filter": [
            {"variant": "paths", "parameter": ["Sunroof.Position"]},
            {"variant": "history", "parameter": "PT1M"}
        ],
        "requestId": "dds-test-get-combined-filter-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-get-combined-filter-req-1");
    const auto& doc = *received_request->request_doc;
    ASSERT_STREQ(doc["action"].GetString(), "get");
    ASSERT_STREQ(doc["path"].GetString(), "Vehicle.Cabin");
    ASSERT_TRUE(doc.HasMember("filter"));
    ASSERT_EQ(doc["filter"].Size(), 2);
}

TEST_F(DdsTransportTest, GetRequestReturnsError) {
    // 1. Prepare the inner VISS request JSON for a non-existent path
    std::string viss_request = R"({
        "action": "get",
        "path": "Vehicle.NonExistent.Path",
        "requestId": "dds-test-get-error-req-1"
    })";

    // 2. Prepare and publish
    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    // 3. Wait for the request to be queued
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-get-error-req-1");
    
    // 4. Simulate sending an error response from the ServiceManager
    auto response_doc = std::make_shared<PooledResponseMap>();
    response_doc->SetObject();
    rapidjson::Document::AllocatorType& allocator = response_doc->GetAllocator();
    response_doc->AddMember("requestId", "dds-test-get-error-req-1", allocator);
     response_doc->AddMember("action", "get", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    error_val.AddMember("reason", "Not Found", allocator);
    response_doc->AddMember("error", error_val, allocator);
    response_doc->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(response_doc);

    // 5. Wait for the response to be published back
    std::unique_lock<std::mutex> lock(response_listener.mtx);
    ASSERT_TRUE(response_listener.cv.wait_for(lock, std::chrono::seconds(1), [&]{ return response_listener.received.load(); }));

    // 6. Verify the error response payload
    rapidjson::Document received_payload;
    received_payload.Parse(response_listener.payload.c_str());
    ASSERT_FALSE(received_payload.HasParseError());
    ASSERT_TRUE(received_payload.HasMember("error"));
    const auto& error = received_payload["error"];
    ASSERT_STREQ(error["number"].GetString(), "404");
}

TEST_F(DdsTransportTest, SetRequestReturnsError) {
    std::string viss_request = R"({
        "action": "set",
        "path": "Vehicle.Invalid.Path",
        "value": "100",
        "requestId": "dds-test-set-error-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-set-error-req-1");

    auto response_doc = std::make_shared<PooledResponseMap>();
    response_doc->SetObject();
    rapidjson::Document::AllocatorType& allocator = response_doc->GetAllocator();
    response_doc->AddMember("requestId", "dds-test-set-error-req-1", allocator);
    response_doc->AddMember("action", "set", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "400", allocator);
    response_doc->AddMember("error", error_val, allocator);
    response_doc->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    received_request->responseChan->push(response_doc);

    std::unique_lock<std::mutex> lock(response_listener.mtx);
    ASSERT_TRUE(response_listener.cv.wait_for(lock, std::chrono::seconds(1), [&]{ return response_listener.received.load(); }));

    rapidjson::Document received_payload;
    received_payload.Parse(response_listener.payload.c_str());
    ASSERT_TRUE(received_payload.HasMember("error"));
}

TEST_F(DdsTransportTest, SubscribeRequestReturnsError) {
    std::string viss_request = R"({
        "action": "subscribe",
        "path": "Vehicle.Invalid.Path",
        "requestId": "dds-test-sub-error-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-sub-error-req-1");

    auto response_doc = std::make_shared<PooledResponseMap>();
    response_doc->SetObject();
    rapidjson::Document::AllocatorType& allocator = response_doc->GetAllocator();
    response_doc->AddMember("requestId", "dds-test-sub-error-req-1", allocator);
    response_doc->AddMember("action", "subscribe", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    response_doc->AddMember("error", error_val, allocator);
    response_doc->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    received_request->responseChan->push(response_doc);

    std::unique_lock<std::mutex> lock(response_listener.mtx);
    ASSERT_TRUE(response_listener.cv.wait_for(lock, std::chrono::seconds(2), [&]{ return response_listener.received.load(); }));

    rapidjson::Document received_payload;
    received_payload.Parse(response_listener.payload.c_str());
    ASSERT_TRUE(received_payload.HasMember("error"));
}

TEST_F(DdsTransportTest, UnsubscribeRequestReturnsError) {
    std::string viss_request = R"({
        "action": "unsubscribe",
        "subscriptionId": "non-existent-sub-id",
        "requestId": "dds-test-unsub-error-req-1"
    })";

    std::string payload_str = CreatePayload(viss_request);
    VissRequestTopic data;
    data.payload(payload_str);
    ASSERT_EQ(request_writer->write(&data), eprosima::fastdds::dds::RETCODE_OK);

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(request_queue->wait_and_pop(received_request));
    ASSERT_EQ(received_request->requestId, "dds-test-unsub-error-req-1");

    auto response_doc = std::make_shared<PooledResponseMap>();
    response_doc->SetObject();
    rapidjson::Document::AllocatorType& allocator = response_doc->GetAllocator();
    response_doc->AddMember("requestId", "dds-test-unsub-error-req-1", allocator);
    response_doc->AddMember("action", "unsubscribe", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    response_doc->AddMember("error", error_val, allocator);
    response_doc->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    received_request->responseChan->push(response_doc);

    std::unique_lock<std::mutex> lock(response_listener.mtx);
    ASSERT_TRUE(response_listener.cv.wait_for(lock, std::chrono::seconds(1), [&]{ return response_listener.received.load(); }));

    rapidjson::Document received_payload;
    received_payload.Parse(response_listener.payload.c_str());
    ASSERT_TRUE(received_payload.HasMember("error"));
}
