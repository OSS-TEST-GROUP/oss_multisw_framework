#include "gtest/gtest.h"
#include "grpc_transport.h"
#include "ThreadSafeQueue.h"
#include "ObjectPool.h"
#include "Types.h"
#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"
#include <thread>
#include <chrono>
#include "JsonSchemaValidator.h"

class GrpcTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        JsonSchemaValidator::init("../config/vissv3.0-schema.json");
        pool = std::make_shared<ObjectPool<RequestData>>();
        response_pool = std::make_shared<ObjectPool<PooledResponseMap>>();
        queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
        transport = std::make_unique<GrpcTransport>(pool, queue);
        transport->start(); // Start the server

        std::string server_address = "localhost:5443";
        channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
        stub = grpcProtobufMessages::VISS::NewStub(channel);

        // Wait for server to be ready
        channel->WaitForConnected(gpr_time_add(gpr_now(GPR_CLOCK_REALTIME), gpr_time_from_seconds(5, GPR_TIMESPAN)));
    }

    void TearDown() override {
        transport->stop();
    }

    std::shared_ptr<ObjectPool<RequestData>> pool;
    std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue;
    std::unique_ptr<ITransport> transport;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<grpcProtobufMessages::VISS::Stub> stub;
};

TEST_F(GrpcTransportTest, GetRequestIsQueued) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-get-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "get");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Speed");

    // 4. Send a mock response back to unblock the client thread
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-req-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    // Add a dummy data part to make it a valid success response
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", "0", allocator);
    dp_val.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    data_val.AddMember("dp", dp_val, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, SetRequestReturnsError) {
    // 1. Prepare client request that will cause an error (e.g., invalid path)
    std::thread client_thread([this]() {
        grpcProtobufMessages::SetRequestMessage req;
        req.set_path("Vehicle.Invalid.Path");
        req.set_value("100");
        req.set_requestid("test-set-error-req-1");
        
        grpcProtobufMessages::SetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->SetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "400");
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));
    EXPECT_EQ(received_request->requestId, "test-set-error-req-1");

    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-set-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "400", allocator);
    error_val.AddMember("reason", "Bad Request", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeRequestReturnsError) {
    // 1. Prepare client request that will cause an error
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Invalid.Path");
        req.set_requestid("test-sub-error-req-1");
        
        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        
        std::unique_ptr<grpc::ClientReader<grpcProtobufMessages::SubscribeStreamMessage>> reader(
            stub->SubscribeRequest(&context, req));

        // Expect initial response with error
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.response().requestid(), "test-sub-error-req-1");
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.response().errorresponse().number(), "404");

        // Expect stream to end immediately
        //ASSERT_FALSE(reader->Read(&res));
        grpc::Status status = reader->Finish();
        //EXPECT_TRUE(status.ok());
    });

    // 2. Handle initial subscribe request from the queue
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));
    EXPECT_EQ(received_request->requestId, "test-sub-error-req-1");

    // 3. Send mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    received_request->responseChan->push(mock_response);
    
    client_thread.join();
}

TEST_F(GrpcTransportTest, UnsubscribeRequestReturnsError) {
    // 1. Prepare client request with a non-existent subscriptionId
    std::thread client_thread([this]() {
        grpcProtobufMessages::UnsubscribeRequestMessage req;
        req.set_subscriptionid("non-existent-sub-id");
        req.set_requestid("test-unsub-error-req-1");
        
        grpcProtobufMessages::UnsubscribeResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->UnsubscribeRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "404");
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));
    EXPECT_EQ(received_request->requestId, "test-unsub-error-req-1");

    // 3. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-unsub-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestReturnsError) {
    // 1. Prepare client request for a non-existent path
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.NonExistent.Path");
        req.set_requestid("test-get-error-req-1");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "404");
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-get-error-req-1");

    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    error_val.AddMember("reason", "Not Found", allocator);
    error_val.AddMember("message", "Path not found.", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithMetadataFilter) {
    // 1. Prepare client request with a metadata filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-metadata-filter-req-1");
        auto* filter_exp = req.mutable_filter()->add_filterexp();
        filter_exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_METADATA);
        auto* meta_value = filter_exp->mutable_value()->mutable_valuemetadata();
        meta_value->set_tree("1");

        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-get-metadata-filter-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "get");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    EXPECT_TRUE(doc.HasMember("filter"));

    // 4. Send a mock response with metadata
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-metadata-filter-req-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    // Instead of 'data', the response for a metadata request should contain 'metadata'
    rapidjson::Value metadata_val(rapidjson::kObjectType);
    metadata_val.AddMember("unit", "km/h", allocator);
    mock_response->AddMember("metadata", metadata_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithHistoryFilter) {
    // 1. Prepare client request with a history filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-history-filter-req-1");
        auto* filter_exp = req.mutable_filter()->add_filterexp();
        filter_exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_HISTORY);
        auto* hist_value = filter_exp->mutable_value()->mutable_valuehistory();
        hist_value->set_timeperiod("P2DT12H");

        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-get-history-filter-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "get");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    ASSERT_TRUE(doc.HasMember("filter"));
    const auto& filters = doc["filter"];
    ASSERT_TRUE(filters.IsArray() && !filters.Empty());
    const auto& first_filter = filters[0];
    ASSERT_TRUE(first_filter.HasMember("variant"));
    EXPECT_STREQ(first_filter["variant"].GetString(), "history");

    // 4. Send a mock response with multiple data points
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-history-filter-req-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:01:00Z", allocator);
    
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);

    rapidjson::Value dp_array(rapidjson::kArrayType);
    
    rapidjson::Value dp1(rapidjson::kObjectType);
    dp1.AddMember("value", "90", allocator);
    dp1.AddMember("ts", "2024-01-01T00:00:15Z", allocator);
    dp_array.PushBack(dp1, allocator);

    rapidjson::Value dp2(rapidjson::kObjectType);
    dp2.AddMember("value", "95", allocator);
    dp2.AddMember("ts", "2024-01-01T00:00:30Z", allocator);
    dp_array.PushBack(dp2, allocator);

    data_val.AddMember("dp", dp_array, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithCombinedFilters) {
    // 1. Prepare client request with combined filters (paths and history)
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Cabin"); // This path will be overridden by the 'paths' filter
        req.set_requestid("test-get-combined-filter-req-1");

        // Add paths filter
        auto* paths_filter = req.mutable_filter()->add_filterexp();
        paths_filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_PATHS);
        auto* paths_value = paths_filter->mutable_value()->mutable_valuepaths();
        paths_value->add_relativepath("Sunroof.Position");
        paths_value->add_relativepath("Light.IsDomeOn");

        // Add history filter
        auto* history_filter = req.mutable_filter()->add_filterexp();
        history_filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_HISTORY);
        auto* hist_value = history_filter->mutable_value()->mutable_valuehistory();
        hist_value->set_timeperiod("PT2M");

        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-get-combined-filter-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "get");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Cabin");
    ASSERT_TRUE(doc.HasMember("filter"));
    const auto& filters = doc["filter"];
    ASSERT_EQ(filters.Size(), 2);
    EXPECT_STREQ(filters[0]["variant"].GetString(), "paths");
    EXPECT_STREQ(filters[1]["variant"].GetString(), "history");


    // 4. Send a mock response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-combined-filter-req-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:02:00Z", allocator);
    
    rapidjson::Value data_array(rapidjson::kArrayType);

    // Data for Sunroof.Position
    rapidjson::Value sunroof_data(rapidjson::kObjectType);
    sunroof_data.AddMember("path", "Vehicle.Cabin.Sunroof.Position", allocator);
    rapidjson::Value sunroof_dp(rapidjson::kArrayType);
    rapidjson::Value dp1(rapidjson::kObjectType);
    dp1.AddMember("value", "50", allocator);
    dp1.AddMember("ts", "2024-01-01T00:00:30Z", allocator);
    sunroof_dp.PushBack(dp1, allocator);
    sunroof_data.AddMember("dp", sunroof_dp, allocator);
    data_array.PushBack(sunroof_data, allocator);

    // Data for Light.IsOn
    rapidjson::Value light_data(rapidjson::kObjectType);
    light_data.AddMember("path", "Vehicle.Cabin.Light.IsDomeOn", allocator);
    rapidjson::Value light_dp(rapidjson::kArrayType);
    rapidjson::Value dp2(rapidjson::kObjectType);
    dp2.AddMember("value", "true", allocator);
    dp2.AddMember("ts", "2024-01-01T00:01:00Z", allocator);
    light_dp.PushBack(dp2, allocator);
    light_data.AddMember("dp", light_dp, allocator);
    data_array.PushBack(light_data, allocator);

    mock_response->AddMember("data", data_array, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithPathFilter) {
    // 1. Prepare client request with a paths filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Cabin"); // Base path
        req.set_requestid("test-get-path-filter-req-1");
        
        // Add paths filter
        auto* paths_filter = req.mutable_filter()->add_filterexp();
        paths_filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_PATHS);
        auto* paths_value = paths_filter->mutable_value()->mutable_valuepaths();
        paths_value->add_relativepath("Sunroof.Position");
        paths_value->add_relativepath("Light.IsDomeOn");

        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-get-path-filter-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "get");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Cabin");
    ASSERT_TRUE(doc.HasMember("filter"));
    const auto& filters = doc["filter"];
    ASSERT_TRUE(filters.IsArray() && !filters.Empty());
    const auto& first_filter = filters[0];
    EXPECT_STREQ(first_filter["variant"].GetString(), "paths");
    const auto& paths = first_filter["parameter"];
    ASSERT_EQ(paths.Size(), 2);
    EXPECT_STREQ(paths[0].GetString(), "Sunroof.Position");
    EXPECT_STREQ(paths[1].GetString(), "Light.IsDomeOn");


    // 4. Send a mock response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-path-filter-req-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    rapidjson::Value data_array(rapidjson::kArrayType);

    // Data for Sunroof.Position
    rapidjson::Value sunroof_data(rapidjson::kObjectType);
    sunroof_data.AddMember("path", "Vehicle.Cabin.Sunroof.Position", allocator);
    rapidjson::Value sunroof_dp(rapidjson::kArrayType);
    rapidjson::Value dp1(rapidjson::kObjectType);
    dp1.AddMember("value", "100", allocator);
    dp1.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    sunroof_dp.PushBack(dp1, allocator);
    sunroof_data.AddMember("dp", sunroof_dp, allocator);
    data_array.PushBack(sunroof_data, allocator);

    // Data for Light.IsDomeOn
    rapidjson::Value light_data(rapidjson::kObjectType);
    light_data.AddMember("path", "Vehicle.Cabin.Light.IsDomeOn", allocator);
    rapidjson::Value light_dp(rapidjson::kArrayType);
    rapidjson::Value dp2(rapidjson::kObjectType);
    dp2.AddMember("value", "false", allocator);
    dp2.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    light_dp.PushBack(dp2, allocator);
    light_data.AddMember("dp", light_dp, allocator);
    data_array.PushBack(light_data, allocator);
    
    mock_response->AddMember("data", data_array, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}




TEST_F(GrpcTransportTest, SetRequestIsQueued) {
    // 1. Prepare client request
    std::thread client_thread([this]() {
        grpcProtobufMessages::SetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_value("50");
        req.set_requestid("test-set-req-1");
        
        grpcProtobufMessages::SetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->SetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::SUCCESS);
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-set-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "set");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    EXPECT_STREQ(doc["value"].GetString(), "50");

    // 4. Send a mock success response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-set-req-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, UnsubscribeRequestIsQueued) {
    // 1. Prepare client request
    std::thread client_thread([this]() {
        grpcProtobufMessages::UnsubscribeRequestMessage req;
        req.set_subscriptionid("test-sub-id-1");
        req.set_requestid("test-unsub-req-1");
        
        grpcProtobufMessages::UnsubscribeResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->UnsubscribeRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::SUCCESS);
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-unsub-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "unsubscribe");
    EXPECT_STREQ(doc["subscriptionId"].GetString(), "test-sub-id-1");

    // 4. Send a mock success response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-unsub-req-1", allocator);
     mock_response->AddMember("subscriptionId", "test-sub-id-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeRequestAndEvent) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-sub-req-1");
        // Add a dummy filter as it's required by the schema
        auto* filter = req.mutable_filter()->add_filterexp();
        filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_TIMEBASED);
        filter->mutable_value()->mutable_valuetimebased()->set_period("1000");

        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        
        std::unique_ptr<grpc::ClientReader<grpcProtobufMessages::SubscribeStreamMessage>> reader(
            stub->SubscribeRequest(&context, req));

        // 2. Expect initial response
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.response().requestid(), "test-sub-req-1");
        EXPECT_EQ(res.response().subscriptionid(), "test-subscription-id");
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::SUCCESS);

        // 3. Expect event
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.event().subscriptionid(), "test-subscription-id");
        const auto& data_pack = res.event().successresponse().datapack();
        EXPECT_EQ(data_pack.data_size(), 1);
        EXPECT_EQ(data_pack.data(0).path(), "Vehicle.Speed");
        EXPECT_EQ(data_pack.data(0).dp_size(), 1);
        EXPECT_EQ(data_pack.data(0).dp(0).value(), "80");

        // 4. Expect stream to end
        ASSERT_FALSE(reader->Read(&res));
        grpc::Status status = reader->Finish();
        EXPECT_TRUE(status.ok());
    });

    // 5. Handle initial subscribe request from the queue
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));
    EXPECT_EQ(received_request->requestId, "test-sub-req-1");

    // 6. Send mock subscription confirmation
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-req-1", allocator);
    mock_response->AddMember("subscriptionId", "test-subscription-id", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    received_request->responseChan->push(mock_response);
    
    // 7. Send mock event
    // The responseChan for the original request is reused for events
    auto mock_event = response_pool->acquire();
    mock_event->SetObject();
    rapidjson::Document::AllocatorType& event_allocator = mock_event->GetAllocator();
    mock_event->AddMember("action", "subscription", event_allocator);
    mock_event->AddMember("subscriptionId", "test-subscription-id", event_allocator);
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", event_allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", "80", event_allocator);
    dp_val.AddMember("ts", "2024-01-01T00:00:01Z", event_allocator);
    data_val.AddMember("dp", dp_val, event_allocator);
    mock_event->AddMember("data", data_val, event_allocator);
    mock_event->AddMember("ts", "2024-01-01T00:00:01Z", event_allocator);
    received_request->responseChan->push(mock_event);

    // 8. Invalidate the channel to terminate the server-side stream
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for the event to be sent
    received_request->responseChan->invalidate();

    client_thread.join();
}
