#include "gtest/gtest.h"
#include "grpc_transport.h"
#include "ThreadSafeQueue.h"
#include "ObjectPool.h"
#include "Types.h"
#include "ConfigManager.h"
#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"
#include <thread>
#include <chrono>
#include "JsonSchemaValidator.h"

class GrpcTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        JsonSchemaValidator::init("../config/vissv3.0-schema.json");
        ConfigManager::getInstance().grpc.port = 55443;
        pool = std::make_shared<ObjectPool<RequestData>>();
        response_pool = std::make_shared<ObjectPool<PooledResponseMap>>();
        queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
        transport = std::make_unique<GrpcTransport>(pool, queue);
        transport->start(); // Start the server

        std::string server_address = "localhost:55443";
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
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
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

TEST_F(GrpcTransportTest, GetRequestAndNoTsResponse) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_TRUE(res.ts().empty());
        
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
    // No "ts" member in this response to test the client's handling
    // mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    // Add a dummy data part to make it a valid success response
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", "0", allocator);
    //dp_val.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    data_val.AddMember("dp", dp_val, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestAndResponseDataPackageTestNoPathNoDp) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        const auto& data_pack = res.successresponse().datapack();
        EXPECT_EQ(data_pack.data_size(), 1);
        EXPECT_TRUE(data_pack.data(0).path().empty());
        EXPECT_EQ(data_pack.data(0).dp_size(), 0);
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
    
    rapidjson::Value data_val(rapidjson::kObjectType);
    //data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", "0", allocator);
    dp_val.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    //data_val.AddMember("dp", dp_val, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestAndResponseDataPackageTestDpNoValueNoTs) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        const auto& data_pack = res.successresponse().datapack();
        EXPECT_EQ(data_pack.data_size(), 1);
        EXPECT_EQ(data_pack.data(0).path(), "Vehicle.Speed");
        EXPECT_EQ(data_pack.data(0).dp_size(), 1);
        EXPECT_TRUE(data_pack.data(0).dp(0).value().empty());
        EXPECT_TRUE(data_pack.data(0).dp(0).ts().empty());
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
    
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    //dp_val.AddMember("value", "0", allocator);
    //dp_val.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    data_val.AddMember("dp", dp_val, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestAndResponseDataPackageTestDpIntValueIntTs) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        const auto& data_pack = res.successresponse().datapack();
        EXPECT_EQ(data_pack.data_size(), 1);
        EXPECT_EQ(data_pack.data(0).path(), "Vehicle.Speed");
        EXPECT_EQ(data_pack.data(0).dp_size(), 1);
        EXPECT_EQ(data_pack.data(0).dp(0).value(), "111");
        EXPECT_TRUE(data_pack.data(0).dp(0).ts().empty());
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
    
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", 111, allocator);
    dp_val.AddMember("ts", 111, allocator);
    data_val.AddMember("dp", dp_val, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestAndResponseDataPackageTestIntPathArrayDp) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        const auto& data_pack = res.successresponse().datapack();
        EXPECT_EQ(data_pack.data_size(), 1);
        EXPECT_TRUE(data_pack.data(0).path().empty());
        EXPECT_EQ(data_pack.data(0).dp_size(), 1);
        
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
    
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", 11111, allocator);
    rapidjson::Value dp_array(rapidjson::kArrayType);
    rapidjson::Value dp_obj(rapidjson::kObjectType);
    dp_obj.AddMember("value", "0", allocator);
    dp_obj.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    dp_array.PushBack(dp_obj, allocator);
    data_val.AddMember("dp", dp_array, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestAndResponseDataPackageTestArrayDpNoValueNoTs) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        const auto& data_pack = res.successresponse().datapack();
        EXPECT_EQ(data_pack.data_size(), 1);
        EXPECT_EQ(data_pack.data(0).path(), "Vehicle.Speed");
        EXPECT_EQ(data_pack.data(0).dp_size(), 1);
        EXPECT_TRUE(data_pack.data(0).dp(0).value().empty());
        EXPECT_TRUE(data_pack.data(0).dp(0).ts().empty());
        
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
    
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_array(rapidjson::kArrayType);
    rapidjson::Value dp_obj(rapidjson::kObjectType);
    //dp_obj.AddMember("value", "0", allocator);
    //dp_obj.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    dp_array.PushBack(dp_obj, allocator);
    data_val.AddMember("dp", dp_array, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestAndResponseDataPackageTestArrayDpIntValueIntTs) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        const auto& data_pack = res.successresponse().datapack();
        EXPECT_EQ(data_pack.data_size(), 1);
        EXPECT_EQ(data_pack.data(0).path(), "Vehicle.Speed");
        EXPECT_EQ(data_pack.data(0).dp_size(), 1);
        EXPECT_EQ(data_pack.data(0).dp(0).value(), "123");
        EXPECT_TRUE(data_pack.data(0).dp(0).ts().empty());
        
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
    
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_array(rapidjson::kArrayType);
    rapidjson::Value dp_obj(rapidjson::kObjectType);
    dp_obj.AddMember("value", 123, allocator);
    dp_obj.AddMember("ts", 12345, allocator);
    dp_array.PushBack(dp_obj, allocator);
    data_val.AddMember("dp", dp_array, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}


TEST_F(GrpcTransportTest, GetRequestAndIntTsResponse) {
    // 1. Prepare client request in a separate thread
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        // This call will block until a response is sent back from the test
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_TRUE(res.ts().empty());
        
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
    // No "ts" member in this response to test the client's handling
    // mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    // Add a dummy data part to make it a valid success response
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", "0", allocator);
    dp_val.AddMember("ts", 1234567, allocator);
    data_val.AddMember("dp", dp_val, allocator);
    mock_response->AddMember("data", data_val, allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

// ################### Comprehensive Error Format Tests ###################

// --- SET Request Error Formats ---

TEST_F(GrpcTransportTest, SetRequestReturnsErrorWithIntegerFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SetRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_value("0");
        req.set_requestid("test-set-int-error-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::SetResponseMessage res;
        grpc::ClientContext context;
        
        stub->SetRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-set-int-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", 400, allocator);
    error_val.AddMember("reason", 401, allocator);
    error_val.AddMember("description", 402, allocator);
    mock_response->AddMember("error", error_val, allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, UnsubscribeRequestReturnsErrorWithMissingFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::UnsubscribeRequestMessage req;
        req.set_subscriptionid("sub-id-missing-fields");
        req.set_requestid("test-unsub-missing-fields-req-1");
        
        grpcProtobufMessages::UnsubscribeResponseMessage res;
        grpc::ClientContext context;
        
        stub->UnsubscribeRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-unsub-missing-fields-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kObjectType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, UnsubscribeRequestReturnsErrorAsArray) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::UnsubscribeRequestMessage req;
        req.set_subscriptionid("sub-id-array-error");
        req.set_requestid("test-unsub-array-error-req-1");
        
        grpcProtobufMessages::UnsubscribeResponseMessage res;
        grpc::ClientContext context;
        
        stub->UnsubscribeRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-unsub-array-error-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kArrayType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeRequestReturnsErrorWithMissingFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_requestid("test-sub-missing-fields-req-1");
        req.set_authorization("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ");
        
        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        auto reader = stub->SubscribeRequest(&context, req);
        
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.response().errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-missing-fields-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kObjectType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeRequestReturnsErrorAsArray) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_requestid("test-sub-array-error-req-1");
        
        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        auto reader = stub->SubscribeRequest(&context, req);
        
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-array-error-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kArrayType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeRequestReturnsErrorAsNonEmptyArray) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_requestid("test-sub-non-empty-array-error-req-1");
        
        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        auto reader = stub->SubscribeRequest(&context, req);
        
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.response().errorresponse().number(), "123");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-non-empty-array-error-req-1", allocator);
    
    rapidjson::Value error_array(rapidjson::kArrayType);
    rapidjson::Value error_obj(rapidjson::kObjectType);
    error_obj.AddMember("number", "123", allocator);
    error_obj.AddMember("reason", "Test Reason", allocator);
    error_obj.AddMember("description", "Test Description", allocator);
    error_array.PushBack(error_obj, allocator);

    mock_response->AddMember("error", error_array, allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

// --- GET Request Error Formats ---

TEST_F(GrpcTransportTest, GetRequestReturnsErrorWithIntegerFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_requestid("test-get-int-error-req-1");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        stub->GetRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-int-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", 400, allocator);
    error_val.AddMember("reason", 401, allocator);
    error_val.AddMember("description", 402, allocator);
    mock_response->AddMember("error", error_val, allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestReturnsErrorWithMissingFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_requestid("test-get-missing-fields-req-1");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        stub->GetRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-missing-fields-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kObjectType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestReturnsErrorAsArray) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_requestid("test-get-array-error-req-1");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        stub->GetRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-array-error-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kArrayType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, SetRequestReturnsErrorWithMissingFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SetRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_value("0");
        req.set_requestid("test-set-missing-fields-req-1");
        
        grpcProtobufMessages::SetResponseMessage res;
        grpc::ClientContext context;
        
        stub->SetRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-set-missing-fields-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kObjectType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, SetRequestReturnsErrorAsArray) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SetRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_value("0");
        req.set_requestid("test-set-array-error-req-1");
        
        grpcProtobufMessages::SetResponseMessage res;
        grpc::ClientContext context;
        
        stub->SetRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-set-array-error-req-1", allocator);
    mock_response->AddMember("error", rapidjson::Value(rapidjson::kArrayType), allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

// --- SUBSCRIBE Request Error Formats ---

TEST_F(GrpcTransportTest, SubscribeRequestReturnsErrorWithIntegerFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Test.Path");
        req.set_requestid("test-sub-int-error-req-1");
        
        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        auto reader = stub->SubscribeRequest(&context, req);
        
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.response().errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-int-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", 500, allocator);
    error_val.AddMember("reason", 501, allocator);
    error_val.AddMember("description", 502, allocator);
    mock_response->AddMember("error", error_val, allocator);
    
    received_request->responseChan->push(mock_response);
    client_thread.join();
}

// --- UNSUBSCRIBE Request Error Formats ---

TEST_F(GrpcTransportTest, UnsubscribeRequestReturnsErrorWithIntegerFields) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::UnsubscribeRequestMessage req;
        req.set_subscriptionid("sub-id-int-error");
        req.set_requestid("test-unsub-int-error-req-1");
        
        grpcProtobufMessages::UnsubscribeResponseMessage res;
        grpc::ClientContext context;
        
        stub->UnsubscribeRequest(&context, req, &res);
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.errorresponse().number(), "");
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-unsub-int-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", 404, allocator);
    error_val.AddMember("reason", 501, allocator);
    error_val.AddMember("description", 502, allocator);
    mock_response->AddMember("error", error_val, allocator);
    
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
        EXPECT_EQ(res.errorresponse().number(), "555");
        EXPECT_EQ(res.errorresponse().description(), "Invalid path");
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
    error_val.AddMember("number", "555", allocator);
    error_val.AddMember("reason", "Bad Request", allocator);
    error_val.AddMember("description", "Invalid path", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, SetRequestWithoutRequestid) {
    // 1. Prepare client request that will cause an error (e.g., invalid path)
    std::thread client_thread([this]() {
        grpcProtobufMessages::SetRequestMessage req;
        req.set_path("Vehicle.Invalid.Path");
        req.set_value("100");
        //req.set_requestid("test-set-error-req-1");
        
        grpcProtobufMessages::SetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->SetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        ASSERT_TRUE(res.requestid().empty());
        EXPECT_EQ(res.errorresponse().number(), "555");
        EXPECT_EQ(res.errorresponse().description(), "Invalid path");
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));
    //EXPECT_EQ(received_request->requestId, "test-set-error-req-1");

    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    //mock_response->AddMember("requestId", "test-set-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "555", allocator);
    error_val.AddMember("reason", "Bad Request", allocator);
    error_val.AddMember("description", "Invalid path", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, SetRequestAndResponseWithIntRequestid) {
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
        ASSERT_TRUE(res.requestid().empty());
        EXPECT_EQ(res.errorresponse().number(), "555");
        EXPECT_EQ(res.errorresponse().description(), "Invalid path");
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));
    EXPECT_EQ(received_request->requestId, "test-set-error-req-1");

    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", 11111, allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "555", allocator);
    error_val.AddMember("reason", "Bad Request", allocator);
    error_val.AddMember("description", "Invalid path", allocator);
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
    error_val.AddMember("reason", "Not Found", allocator);
    error_val.AddMember("description", "description", allocator);
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
    error_val.AddMember("description", "Path not found.", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithoutRequestid) {
    // 1. Prepare client request for a non-existent path
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.NonExistent.Path");
        //req.set_requestid("test-get-error-req-1");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        ASSERT_TRUE(res.requestid().empty());
        EXPECT_EQ(res.errorresponse().number(), "404");
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_TRUE(received_request->requestId.empty());

    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "get");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.NonExistent.Path");
    EXPECT_FALSE(doc.HasMember("requestId"));



    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    //mock_response->AddMember("requestId", "test-get-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    error_val.AddMember("reason", "Not Found", allocator);
    error_val.AddMember("description", "Path not found.", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestAndResponseWithIntRequestid) {
    // 1. Prepare client request for a non-existent path
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.NonExistent.Path");
        //req.set_requestid("test-get-error-req-1");
        
        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;
        
        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        ASSERT_TRUE(res.requestid().empty());
        EXPECT_EQ(res.errorresponse().number(), "404");
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_TRUE(received_request->requestId.empty());

    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "get");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.NonExistent.Path");
    EXPECT_FALSE(doc.HasMember("requestId"));



    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", 11111, allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    error_val.AddMember("reason", "Not Found", allocator);
    error_val.AddMember("description", "Path not found.", allocator);
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
    ASSERT_TRUE(doc.HasMember("filter"));

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

TEST_F(GrpcTransportTest, GetRequestWithMetadataFilterNoParam) {
    // 1. Prepare client request with a metadata filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-metadata-filter-req-1");
        auto* filter_exp = req.mutable_filter()->add_filterexp();
        filter_exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_METADATA);
        //auto* meta_value = filter_exp->mutable_value()->mutable_valuemetadata();
        //meta_value->set_tree("1");

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
    const auto& filters = doc["filter"];
    ASSERT_TRUE(filters.IsArray() && !filters.Empty());
    const auto& first_filter = filters[0];
    EXPECT_FALSE(first_filter.HasMember("parameter"));

    
 // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-metadata-filter-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    error_val.AddMember("reason", "Not Found", allocator);
    error_val.AddMember("description", "Path not found.", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
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

TEST_F(GrpcTransportTest, GetRequestWithHistoryFilterNoParam) {
    // 1. Prepare client request with a history filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-history-filter-req-1");
        auto* filter_exp = req.mutable_filter()->add_filterexp();
        filter_exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_HISTORY);
        //auto* hist_value = filter_exp->mutable_value()->mutable_valuehistory();
        //hist_value->set_timeperiod("P2DT12H");

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
    EXPECT_FALSE(first_filter.HasMember("parameter"));

    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-metadata-filter-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    error_val.AddMember("reason", "Not Found", allocator);
    error_val.AddMember("description", "Path not found.", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
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


TEST_F(GrpcTransportTest, GetRequestWithPathFilterNoParam) {
    // 1. Prepare client request with a paths filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Cabin"); // Base path
        req.set_requestid("test-get-path-filter-req-1");
        
        // Add paths filter
        auto* paths_filter = req.mutable_filter()->add_filterexp();
        paths_filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_PATHS);
        //auto* paths_value = paths_filter->mutable_value()->mutable_valuepaths();
        //paths_value->add_relativepath("Sunroof.Position");
        //paths_value->add_relativepath("Light.IsDomeOn");

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
    ASSERT_FALSE(doc.HasMember("parameter"));
    
        // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-path-filter-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    error_val.AddMember("reason", "Not Found", allocator);
    error_val.AddMember("description", "Path not found.", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();

}


TEST_F(GrpcTransportTest, GetRequestWithTimebasedFilterNoParam) {
    // 1. Prepare client request with a paths filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Cabin"); // Base path
        req.set_requestid("test-get-path-filter-req-1");
        // Add paths filter
        auto* paths_filter = req.mutable_filter()->add_filterexp();
        paths_filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_TIMEBASED);

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
    ASSERT_FALSE(doc.HasMember("parameter"));
    
        // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-path-filter-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}


TEST_F(GrpcTransportTest, GetRequestWithRangeFilterNoParam) {
    // 1. Prepare client request with a paths filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Cabin"); // Base path
        req.set_requestid("test-get-path-filter-req-1");
        // Add paths filter
        auto* paths_filter = req.mutable_filter()->add_filterexp();
        paths_filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_RANGE);

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
    ASSERT_FALSE(doc.HasMember("parameter"));
    
        // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-path-filter-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithChangeFilterNoParam) {
    // 1. Prepare client request with a paths filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Cabin"); // Base path
        req.set_requestid("test-get-path-filter-req-1");
        // Add paths filter
        auto* paths_filter = req.mutable_filter()->add_filterexp();
        paths_filter->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_CHANGE);

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
    ASSERT_FALSE(doc.HasMember("parameter"));
    
        // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-path-filter-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "404", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
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

TEST_F(GrpcTransportTest, SetRequestResponseNoTs) {
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
        EXPECT_TRUE(res.ts().empty());
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
    //mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, SetRequestResponseIntTs) {
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
        EXPECT_TRUE(res.ts().empty());
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
    mock_response->AddMember("ts", 12345, allocator);
    
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

TEST_F(GrpcTransportTest, UnsubscribeRequestAndResponseNoTs) {
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
        EXPECT_TRUE(res.ts().empty());
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
    //mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, UnsubscribeRequestAndResponseIntTs) {
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
        EXPECT_TRUE(res.ts().empty());
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
    mock_response->AddMember("ts", 12345, allocator);
    
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

TEST_F(GrpcTransportTest, SubscribeRequestAndEventNoTs) {
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
        EXPECT_TRUE(res.response().ts().empty());

        // 3. Expect event
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.event().subscriptionid(), "test-subscription-id");
        EXPECT_TRUE(res.event().ts().empty());
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
    //mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
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
    //mock_event->AddMember("ts", "2024-01-01T00:00:01Z", event_allocator);
    received_request->responseChan->push(mock_event);

    // 8. Invalidate the channel to terminate the server-side stream
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for the event to be sent
    received_request->responseChan->invalidate();

    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeRequestAndEventIntTs) {
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
        EXPECT_TRUE(res.response().ts().empty());

        // 3. Expect event
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.event().subscriptionid(), "test-subscription-id");
        EXPECT_TRUE(res.event().ts().empty());
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
    mock_response->AddMember("ts", 12345, allocator);
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
    mock_event->AddMember("ts", 56789, event_allocator);
    received_request->responseChan->push(mock_event);

    // 8. Invalidate the channel to terminate the server-side stream
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for the event to be sent
    received_request->responseChan->invalidate();

    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeRequestAndEventArrayData) {
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
        EXPECT_EQ(data_pack.data_size(), 2);
        EXPECT_EQ(data_pack.data(0).path(), "Vehicle.Speed");
        EXPECT_EQ(data_pack.data(0).dp_size(), 1);
        EXPECT_EQ(data_pack.data(0).dp(0).value(), "80");
        EXPECT_EQ(data_pack.data(1).path(), "Vehicle.Height");

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
    rapidjson::Value data_array(rapidjson::kArrayType);
    
    // First data element
    rapidjson::Value data_val1(rapidjson::kObjectType);
    data_val1.AddMember("path", "Vehicle.Speed", event_allocator);
    rapidjson::Value dp_val1(rapidjson::kObjectType);
    dp_val1.AddMember("value", "80", event_allocator);
    dp_val1.AddMember("ts", "2024-01-01T00:00:01Z", event_allocator);
    data_val1.AddMember("dp", dp_val1, event_allocator);
    data_array.PushBack(data_val1, event_allocator);

    // Second data element
    rapidjson::Value data_val2(rapidjson::kObjectType);
    data_val2.AddMember("path", "Vehicle.Height", event_allocator);
    rapidjson::Value dp_val2(rapidjson::kObjectType);
    dp_val2.AddMember("value", "88", event_allocator);
    dp_val2.AddMember("ts", "2024-01-01T00:00:01Z", event_allocator);
    data_val2.AddMember("dp", dp_val2, event_allocator);
    data_array.PushBack(data_val2, event_allocator);

    mock_event->AddMember("data", data_array, event_allocator);
    mock_event->AddMember("ts", "2024-01-01T00:00:01Z", event_allocator);
    received_request->responseChan->push(mock_event);

    // 8. Invalidate the channel to terminate the server-side stream
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for the event to be sent
    received_request->responseChan->invalidate();

    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeWithUnimplementedCurvelogFilterReturnsError) {
    // 1. Prepare client request with an unsupported curvelog filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-sub-curvelog-error-req-1");
        
        auto* filter_exp = req.mutable_filter()->add_filterexp();
        filter_exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_CURVELOG);
        auto* curvelog_value = filter_exp->mutable_value()->mutable_valuecurvelog();
        curvelog_value->set_maxerr("0.1");
        curvelog_value->set_bufsize("10");

        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        
        std::unique_ptr<grpc::ClientReader<grpcProtobufMessages::SubscribeStreamMessage>> reader(
            stub->SubscribeRequest(&context, req));

        // Expect initial response with error
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.response().requestid(), "test-sub-curvelog-error-req-1");
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.response().errorresponse().number(), "400");
        
        // Expect stream to end
        grpc::Status status = reader->Finish();
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-sub-curvelog-error-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "subscribe");
    
    // As per feedback, the filter should be empty since it's not implemented
    ASSERT_TRUE(doc.HasMember("filter"));
    const auto& filters = doc["filter"];
    ASSERT_TRUE(filters.IsArray());
    EXPECT_TRUE(filters.Empty());

    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-curvelog-error-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "400", allocator);
    error_val.AddMember("reason", "Bad Request", allocator);
    error_val.AddMember("description", "Unsupported filter: curvelog", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeWithRangeFilter) {
    // 1. Prepare client request with a range filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-sub-range-filter-req-1");
        
        auto* filter_exp = req.mutable_filter()->add_filterexp();
        filter_exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_RANGE);
        auto* range_value = filter_exp->mutable_value()->add_valuerange();
        range_value->set_logicoperator("eq");
        range_value->set_boundary("100");

        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        
        std::unique_ptr<grpc::ClientReader<grpcProtobufMessages::SubscribeStreamMessage>> reader(
            stub->SubscribeRequest(&context, req));

        // Expect initial response
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.response().subscriptionid(), "test-sub-range-id");
        
        // Invalidate stream
        context.TryCancel();
        reader->Finish();
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-sub-range-filter-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "subscribe");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    ASSERT_TRUE(doc.HasMember("filter"));
    const auto& filters = doc["filter"];
    ASSERT_TRUE(filters.IsArray() && !filters.Empty());
    const auto& first_filter = filters[0];
    ASSERT_TRUE(first_filter.HasMember("variant"));
    EXPECT_STREQ(first_filter["variant"].GetString(), "range");
    ASSERT_TRUE(first_filter.HasMember("parameter"));
    const auto& param = first_filter["parameter"];
    ASSERT_TRUE(param.IsArray());
    const auto& first_param = param[0];

    ASSERT_TRUE(first_param.HasMember("logic-op"));
    ASSERT_TRUE(first_param.HasMember("boundary"));

    // 4. Send a mock success response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-range-filter-req-1", allocator);
    mock_response->AddMember("subscriptionId", "test-sub-range-id", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);
    
    // Invalidate the channel to terminate the server-side stream
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for the response to be sent
    received_request->responseChan->invalidate();

    client_thread.join();
}



TEST_F(GrpcTransportTest, SubscribeWithChangeFilterReturnsError) {
    // 1. Prepare client request with a change filter
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-sub-change-filter-req-1");
        
        auto* filter_exp = req.mutable_filter()->add_filterexp();
        filter_exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_CHANGE);
        auto* change_value = filter_exp->mutable_value()->mutable_valuechange();
        change_value->set_logicoperator("ne");
        change_value->set_diff("0");

        grpcProtobufMessages::SubscribeStreamMessage res;
        grpc::ClientContext context;
        
        std::unique_ptr<grpc::ClientReader<grpcProtobufMessages::SubscribeStreamMessage>> reader(
            stub->SubscribeRequest(&context, req));

        // Expect initial response with error
        ASSERT_TRUE(reader->Read(&res));
        EXPECT_EQ(res.response().requestid(), "test-sub-change-filter-req-1");
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::ERROR);
        EXPECT_EQ(res.response().errorresponse().number(), "400");
        
        // Expect stream to end
        grpc::Status status = reader->Finish();
    });

    // 2. Wait for the server to process and queue the request
    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // 3. Verify the request content
    EXPECT_EQ(received_request->requestId, "test-sub-change-filter-req-1");
    const auto& doc = *received_request->request_doc;
    EXPECT_STREQ(doc["action"].GetString(), "subscribe");
    EXPECT_STREQ(doc["path"].GetString(), "Vehicle.Speed");
    ASSERT_TRUE(doc.HasMember("filter"));
    const auto& filters = doc["filter"];
    ASSERT_TRUE(filters.IsArray() && !filters.Empty());
    const auto& first_filter = filters[0];
    ASSERT_TRUE(first_filter.HasMember("variant"));
    EXPECT_STREQ(first_filter["variant"].GetString(), "change");
    ASSERT_TRUE(first_filter.HasMember("parameter"));
    const auto& param = first_filter["parameter"];
    ASSERT_TRUE(param.IsArray());
    const auto& first_param = param[0];

    ASSERT_TRUE(first_param.HasMember("logic-op"));
    ASSERT_TRUE(first_param.HasMember("diff"));

    // 4. Send a mock error response
    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-sub-change-filter-req-1", allocator);
    rapidjson::Value error_val(rapidjson::kObjectType);
    error_val.AddMember("number", "400", allocator);
    error_val.AddMember("reason", "Bad Request", allocator);
    error_val.AddMember("description", "Invalid filter specified.", allocator);
    mock_response->AddMember("error", error_val, allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    
    received_request->responseChan->push(mock_response);

    // Invalidate the channel to terminate the server-side stream
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for the response to be sent
    received_request->responseChan->invalidate();

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithMetadataResponse) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-meta-1");

        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;

        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(res.status(), grpcProtobufMessages::ResponseStatus::SUCCESS);
        EXPECT_TRUE(res.has_successresponse());
        EXPECT_FALSE(res.successresponse().metadata().empty());
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-meta-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);

    rapidjson::Value meta_val(rapidjson::kObjectType);
    meta_val.AddMember("type", "sensor", allocator);
    meta_val.AddMember("datatype", "float", allocator);
    mock_response->AddMember("metadata", meta_val, allocator);

    received_request->responseChan->push(mock_response);
    client_thread.join();
}

TEST_F(GrpcTransportTest, SubscribeWithServerTerminationStatus) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::SubscribeRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-sub-term-1");

        grpc::ClientContext context;
        auto reader = stub->SubscribeRequest(&context, req);

        grpcProtobufMessages::SubscribeStreamMessage initial_msg;
        EXPECT_TRUE(reader->Read(&initial_msg));
        EXPECT_EQ(initial_msg.status(), grpcProtobufMessages::ResponseStatus::SUCCESS);

        grpcProtobufMessages::SubscribeStreamMessage event_msg;
        EXPECT_TRUE(reader->Read(&event_msg));

        // Read until stream ends (server termination)
        grpcProtobufMessages::SubscribeStreamMessage dummy;
        while (reader->Read(&dummy)) {}
        reader->Finish();
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    // Send initial subscribe success
    auto initial_resp = response_pool->acquire();
    initial_resp->SetObject();
    rapidjson::Document::AllocatorType& allocator = initial_resp->GetAllocator();
    initial_resp->AddMember("requestId", "test-sub-term-1", allocator);
    initial_resp->AddMember("subscriptionId", "sub-term-123", allocator);
    initial_resp->AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    received_request->responseChan->push(initial_resp);

    // Send one data event
    auto event_doc = response_pool->acquire();
    event_doc->SetObject();
    event_doc->AddMember("subscriptionId", "sub-term-123", allocator);
    event_doc->AddMember("ts", "2024-01-01T00:00:01Z", allocator);
    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_val(rapidjson::kObjectType);
    dp_val.AddMember("value", "65.5", allocator);
    dp_val.AddMember("ts", "2024-01-01T00:00:01Z", allocator);
    data_val.AddMember("dp", dp_val, allocator);
    event_doc->AddMember("data", data_val, allocator);
    received_request->responseChan->push(event_doc);

    // Send terminated event
    auto term_doc = response_pool->acquire();
    term_doc->SetObject();
    term_doc->AddMember("subscriptionId", "sub-term-123", allocator);
    term_doc->AddMember("status", "terminated", allocator);
    received_request->responseChan->push(term_doc);

    client_thread.join();
}

TEST_F(GrpcTransportTest, GetRequestWithHistoryFilterParam) {
    std::thread client_thread([this]() {
        grpcProtobufMessages::GetRequestMessage req;
        req.set_path("Vehicle.Speed");
        req.set_requestid("test-get-history-param-1");

        auto filter = req.mutable_filter();
        auto exp = filter->add_filterexp();
        exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_HISTORY);
        auto val = exp->mutable_value();
        auto hist = val->mutable_valuehistory();
        hist->set_timeperiod("PT5S");

        grpcProtobufMessages::GetResponseMessage res;
        grpc::ClientContext context;

        grpc::Status status = stub->GetRequest(&context, req, &res);
        EXPECT_TRUE(status.ok());
    });

    std::shared_ptr<RequestData> received_request;
    ASSERT_TRUE(queue->wait_and_pop(received_request));

    auto mock_response = response_pool->acquire();
    mock_response->SetObject();
    rapidjson::Document::AllocatorType& allocator = mock_response->GetAllocator();
    mock_response->AddMember("requestId", "test-get-history-param-1", allocator);
    mock_response->AddMember("ts", "2024-01-01T00:00:00Z", allocator);

    rapidjson::Value data_val(rapidjson::kObjectType);
    data_val.AddMember("path", "Vehicle.Speed", allocator);
    rapidjson::Value dp_arr(rapidjson::kArrayType);
    rapidjson::Value dp_item(rapidjson::kObjectType);
    dp_item.AddMember("value", "80", allocator);
    dp_item.AddMember("ts", "2024-01-01T00:00:00Z", allocator);
    dp_arr.PushBack(dp_item, allocator);
    data_val.AddMember("dp", dp_arr, allocator);
    mock_response->AddMember("data", data_val, allocator);

    received_request->responseChan->push(mock_response);
    client_thread.join();
}

