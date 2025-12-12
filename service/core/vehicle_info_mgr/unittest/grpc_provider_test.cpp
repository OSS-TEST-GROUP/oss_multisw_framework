#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "GrpcProvider.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include "VISSv3.0.grpc.pb.h"
#include <grpcpp/test/mock_stream.h>
#include <thread>
#include <chrono>
#include <future>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::DoAll;

// Mock for the gRPC VISS Stub
class MockVissStub : public grpcProtobufMessages::VISS::StubInterface {
public:
    MOCK_METHOD(grpc::Status, GetRequest, (grpc::ClientContext* context, const grpcProtobufMessages::GetRequestMessage& request, grpcProtobufMessages::GetResponseMessage* response), (override));
    MOCK_METHOD(grpc::Status, SetRequest, (grpc::ClientContext* context, const grpcProtobufMessages::SetRequestMessage& request, grpcProtobufMessages::SetResponseMessage* response), (override));
    MOCK_METHOD(grpc::Status, UnsubscribeRequest, (grpc::ClientContext* context, const grpcProtobufMessages::UnsubscribeRequestMessage& request, grpcProtobufMessages::UnsubscribeResponseMessage* response), (override));
    MOCK_METHOD(grpc::ClientReaderInterface<grpcProtobufMessages::SubscribeStreamMessage>*, SubscribeRequestRaw, (grpc::ClientContext* context, const grpcProtobufMessages::SubscribeRequestMessage& request), (override));

    // Mock the Raw methods as well, returning nullptr for async versions as they are not used in this test.
    grpc::ClientAsyncResponseReaderInterface<grpcProtobufMessages::GetResponseMessage>* AsyncGetRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::GetRequestMessage& request, grpc::CompletionQueue* cq) override { return nullptr; }
    grpc::ClientAsyncResponseReaderInterface<grpcProtobufMessages::GetResponseMessage>* PrepareAsyncGetRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::GetRequestMessage& request, grpc::CompletionQueue* cq) override { return nullptr; }
    grpc::ClientAsyncResponseReaderInterface<grpcProtobufMessages::SetResponseMessage>* AsyncSetRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::SetRequestMessage& request, grpc::CompletionQueue* cq) override { return nullptr; }
    grpc::ClientAsyncResponseReaderInterface<grpcProtobufMessages::SetResponseMessage>* PrepareAsyncSetRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::SetRequestMessage& request, grpc::CompletionQueue* cq) override { return nullptr; }
    grpc::ClientAsyncReaderInterface<grpcProtobufMessages::SubscribeStreamMessage>* AsyncSubscribeRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::SubscribeRequestMessage& request, grpc::CompletionQueue* cq, void* tag) override { return nullptr; }
    grpc::ClientAsyncReaderInterface<grpcProtobufMessages::SubscribeStreamMessage>* PrepareAsyncSubscribeRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::SubscribeRequestMessage& request, grpc::CompletionQueue* cq) override { return nullptr; }
    grpc::ClientAsyncResponseReaderInterface<grpcProtobufMessages::UnsubscribeResponseMessage>* AsyncUnsubscribeRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::UnsubscribeRequestMessage& request, grpc::CompletionQueue* cq) override { return nullptr; }
    grpc::ClientAsyncResponseReaderInterface<grpcProtobufMessages::UnsubscribeResponseMessage>* PrepareAsyncUnsubscribeRequestRaw(grpc::ClientContext* context, const grpcProtobufMessages::UnsubscribeRequestMessage& request, grpc::CompletionQueue* cq) override { return nullptr; }
};

// Mock for the gRPC Channel
class MockChannel : public grpc::ChannelInterface {
public:
    MOCK_METHOD(grpc_connectivity_state, GetState, (bool try_to_connect), (override));
    MOCK_METHOD(grpc::internal::Call, CreateCall, (const grpc::internal::RpcMethod& method, grpc::ClientContext* context, grpc::CompletionQueue* cq), (override));
    MOCK_METHOD(void, PerformOpsOnCall, (grpc::internal::CallOpSetInterface* ops, grpc::internal::Call* call), (override));
    MOCK_METHOD(void*, RegisterMethod, (const char* method), (override));

private:
    // These are pure virtual in the base class and must be implemented.
    // We provide dummy implementations as they are not used in these tests.
    void NotifyOnStateChangeImpl(grpc_connectivity_state, gpr_timespec,
                                 grpc::CompletionQueue*, void*) override {}
    bool WaitForStateChangeImpl(grpc_connectivity_state,
                                gpr_timespec) override {
      return false;
    }
};


class GrpcProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto mock_stub_unique = std::make_unique<testing::NiceMock<MockVissStub>>();
        mock_stub_ptr = mock_stub_unique.get();
        
        mock_channel = std::make_shared<testing::NiceMock<MockChannel>>();
        
        // Default behavior for the channel is to be ready.
        ON_CALL(*mock_channel, GetState(_)).WillByDefault(Return(GRPC_CHANNEL_READY));

        grpc_provider = std::make_unique<GrpcProvider>(std::move(mock_stub_unique), mock_channel);
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
        grpc_provider->setDataChannel(data_channel);
    }

    void TearDown() override {
        if (grpc_provider) {
            grpc_provider->stop();
            grpc_provider.reset();
        }
    }

    std::unique_ptr<GrpcProvider> grpc_provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
    MockVissStub* mock_stub_ptr;
    std::shared_ptr<testing::NiceMock<MockChannel>> mock_channel;
};

TEST_F(GrpcProviderTest, SetValueSuccess) {
    // Expect the background subscription call to fail (e.g., server error)
    // and return a nullptr stream. The provider should handle this gracefully.
    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.*")))
        .WillOnce(Return(nullptr));

    grpc_provider->start();

    std::promise<void> promise;
    auto future = promise.get_future();

    grpcProtobufMessages::SetRequestMessage received_request;
    EXPECT_CALL(*mock_stub_ptr, SetRequest(_, _, _))
        .Times(1)
        .WillOnce(DoAll(
            SaveArg<1>(&received_request),
            [&promise]() { promise.set_value(); },
            Return(grpc::Status::OK)
        ));

    grpc_provider->setValue("Vehicle.Speed", std::string("80"));
    
    // Wait for the SetRequest call to complete to ensure received_request is populated.
    ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);

    EXPECT_EQ(received_request.path(), "Vehicle.Speed");
    EXPECT_EQ(received_request.value(), "80");
}

TEST_F(GrpcProviderTest, SetValueBadAnyCast) {
    // This test verifies that calling setValue with a non-string 'any'
    // is handled gracefully without crashing.
    IProvider* provider = grpc_provider.get();

    // The implementation should catch the std::bad_any_cast and log an error.
    // The test passes if no unhandled exception is thrown.
    ASSERT_NO_THROW(provider->setValue("Vehicle.Speed", 80));
}

TEST_F(GrpcProviderTest, SetValueError) {
   EXPECT_CALL(*mock_channel, GetState(true))
    .WillRepeatedly(Return(GRPC_CHANNEL_TRANSIENT_FAILURE));

    grpc_provider->start();

    grpcProtobufMessages::SetRequestMessage received_request;
    EXPECT_CALL(*mock_stub_ptr, SetRequest(_, _, _))
        .Times(1)
        .WillOnce(DoAll(
            SaveArg<1>(&received_request),
            Return(grpc::Status(grpc::StatusCode::UNKNOWN, "Test error"))
        ));

    ASSERT_NO_THROW(grpc_provider->setValue("Vehicle.Speed", std::string("80")));
    // The stop is called in TearDown, so if we reach here, it's a pass.
    //SUCCEED();
    EXPECT_EQ(received_request.path(), "Vehicle.Speed");
    EXPECT_EQ(received_request.value(), "80");
}

TEST_F(GrpcProviderTest, SubscribeReceivesData) {
    auto* stream_reader = new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>();
    
    // 1. First, the success response with subscription ID
    grpcProtobufMessages::SubscribeStreamMessage success_msg;
    success_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    success_msg.mutable_response()->set_subscriptionid("test-sub-123");

    // 2. Then, the data event
    grpcProtobufMessages::SubscribeStreamMessage event_msg;
    event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
    auto* sub_event = event_msg.mutable_event();
    auto* success_resp = sub_event->mutable_successresponse();
    auto* data_pack = success_resp->mutable_datapack();
    auto* data1 = data_pack->add_data();
    data1->set_path("Vehicle.Speed");
    data1->add_dp()->set_value("100");

    EXPECT_CALL(*stream_reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(success_msg), Return(true))) // Send success response
        .WillOnce(DoAll(SetArgPointee<0>(event_msg), Return(true)))   // Send data event
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.*")))
        .Times(1)
        .WillOnce(Return(stream_reader));

    grpc_provider->start();
    //grpc_provider->subscribe("Vehicle.Speed");
    
    VehicleData received_data;
    ASSERT_TRUE(data_channel->wait_and_pop(received_data));
    EXPECT_EQ(received_data.path, "Vehicle.Speed");
    EXPECT_EQ(std::any_cast<std::string>(received_data.value), "100");
}

TEST_F(GrpcProviderTest, SubscribeAndEventResponse) {
    EXPECT_CALL(*mock_channel, GetState(true))
    .WillRepeatedly(Return(GRPC_CHANNEL_TRANSIENT_FAILURE));
    
    auto* stream_reader = new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>();
    
    // 1. First, the success response with subscription ID
    grpcProtobufMessages::SubscribeStreamMessage success_msg;
    success_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
    success_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    success_msg.mutable_response()->set_subscriptionid("test-sub-123");

    // 2. Then, the data event
    /*
    grpcProtobufMessages::SubscribeStreamMessage event_msg;
    event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
    auto* sub_event = event_msg.mutable_event();
    auto* success_resp = sub_event->mutable_successresponse();
    auto* data_pack = success_resp->mutable_datapack();
    auto* data1 = data_pack->add_data();
    data1->set_path("Vehicle.Speed");
    data1->add_dp()->set_value("100");
    */

    EXPECT_CALL(*stream_reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(success_msg), Return(true))) // Send success response
        //.WillOnce(DoAll(SetArgPointee<0>(event_msg), Return(true)))   // Send data event
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.Speed")))
        .Times(1)
        .WillOnce(Return(stream_reader));

    grpc_provider->start();
    ASSERT_NO_THROW(grpc_provider->subscribe("Vehicle.Speed"));
}

TEST_F(GrpcProviderTest, SubscribeAndNoResponse) {
    EXPECT_CALL(*mock_channel, GetState(true))
    .WillRepeatedly(Return(GRPC_CHANNEL_TRANSIENT_FAILURE));
    
    auto* stream_reader = new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>();
    
    // 1. First, the success response with subscription ID
    grpcProtobufMessages::SubscribeStreamMessage success_msg;
      success_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::RESPONSE);
    success_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    //success_msg.mutable_response()->set_subscriptionid("test-sub-123");

    // 2. Then, the data event
    /*
    grpcProtobufMessages::SubscribeStreamMessage event_msg;
    event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
    auto* sub_event = event_msg.mutable_event();
    auto* success_resp = sub_event->mutable_successresponse();
    auto* data_pack = success_resp->mutable_datapack();
    auto* data1 = data_pack->add_data();
    data1->set_path("Vehicle.Speed");
    data1->add_dp()->set_value("100");
    */

    EXPECT_CALL(*stream_reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(success_msg), Return(true))) // Send success response
        //.WillOnce(DoAll(SetArgPointee<0>(event_msg), Return(true)))   // Send data event
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.Speed")))
        .Times(1)
        .WillOnce(Return(stream_reader));

    grpc_provider->start();
    ASSERT_NO_THROW(grpc_provider->subscribe("Vehicle.Speed"));
}


TEST_F(GrpcProviderTest, SubscribeTwice) {
   EXPECT_CALL(*mock_channel, GetState(true))
    .WillRepeatedly(Return(GRPC_CHANNEL_TRANSIENT_FAILURE));

    // 1. First, successfully subscribe to a path.
    auto* stream_reader = new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>();
    grpcProtobufMessages::SubscribeStreamMessage success_msg;
    success_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    success_msg.mutable_response()->set_subscriptionid("sub-to-unsub");

    grpcProtobufMessages::SubscribeStreamMessage event_msg;
    event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
    auto* sub_event = event_msg.mutable_event();
    auto* success_resp = sub_event->mutable_successresponse();
    auto* data_pack = success_resp->mutable_datapack();
    auto* data1 = data_pack->add_data();
    data1->set_path("Vehicle.Temp");
    data1->add_dp()->set_value("100");

    // The mock stream should send the success response and then can appear closed.
    EXPECT_CALL(*stream_reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(success_msg), Return(true)))
        .WillRepeatedly(DoAll(SetArgPointee<0>(event_msg), Return(true)));

    // Expect the call for the specific subscription
    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.Temp")))
        .Times(1)
        .WillOnce(Return(stream_reader));

    // Also expect the background subscription from the monitor loop
    //EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.*")))
    //    .WillOnce(Return(new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>()));

    grpc_provider->start();
    grpc_provider->subscribe("Vehicle.Temp");

    // Give a moment for the subscription thread to start and process the response
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 3. Call subscribe.
    grpc_provider->subscribe("Vehicle.Temp");

}

TEST_F(GrpcProviderTest, SubscribeFailsWithErrorResponse) {
    // Background subscription from monitor loop
    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.*")))
        .WillOnce(Return(new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>()));

    auto* error_stream_reader = new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>();
    
    // The stream returns a single error message and then closes.
    grpcProtobufMessages::SubscribeStreamMessage error_msg;
    error_msg.set_status(grpcProtobufMessages::ResponseStatus::ERROR);
    error_msg.mutable_response()->mutable_errorresponse()->set_reason("Invalid Path");

    EXPECT_CALL(*error_stream_reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(error_msg), Return(true)))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.InvalidPath")))
        .Times(1)
        .WillOnce(Return(error_stream_reader));

    grpc_provider->start();
    grpc_provider->subscribe("Vehicle.InvalidPath");

    // Verify that no subscription was actually created and no data is pushed.
    ASSERT_TRUE(data_channel->empty());
}

TEST_F(GrpcProviderTest, ReconnectLogic) {
    // Simulate the channel being down initially, then coming up, then going down again.
    EXPECT_CALL(*mock_channel, GetState(true))
        .WillOnce(Return(GRPC_CHANNEL_TRANSIENT_FAILURE))
        .WillOnce(Return(GRPC_CHANNEL_READY))
        .WillRepeatedly(Return(GRPC_CHANNEL_TRANSIENT_FAILURE));

    // Expect SubscribeRequestRaw to be called when the channel becomes ready.
    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.*")))
        .Times(1)
        .WillOnce(Return(new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>()));

    grpc_provider->start();

    // Give the monitor thread time to run and attempt reconnection.
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

TEST_F(GrpcProviderTest, UnsubscribeSuccess) {
    EXPECT_CALL(*mock_channel, GetState(true))
    .WillRepeatedly(Return(GRPC_CHANNEL_TRANSIENT_FAILURE));

    // 1. First, successfully subscribe to a path.
    auto* stream_reader = new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>();
    grpcProtobufMessages::SubscribeStreamMessage success_msg;
    success_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    success_msg.mutable_response()->set_subscriptionid("sub-to-unsub");


    grpcProtobufMessages::SubscribeStreamMessage event_msg;
    event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
    auto* sub_event = event_msg.mutable_event();
    auto* success_resp = sub_event->mutable_successresponse();
    auto* data_pack = success_resp->mutable_datapack();
    auto* data1 = data_pack->add_data();
    data1->set_path("Vehicle.Temp");
    data1->add_dp()->set_value("100");

    // The mock stream should send the success response and then can appear closed.
    EXPECT_CALL(*stream_reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(success_msg), Return(true)))
        .WillRepeatedly(DoAll(SetArgPointee<0>(event_msg), Return(true)));

    // Expect the call for the specific subscription
    EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.Temp")))
        .Times(1)
        .WillOnce(Return(stream_reader));

    // Also expect the background subscription from the monitor loop
    //EXPECT_CALL(*mock_stub_ptr, SubscribeRequestRaw(_, ::testing::Property(&grpcProtobufMessages::SubscribeRequestMessage::path, "Vehicle.*")))
    //    .WillOnce(Return(new ::grpc::testing::MockClientReader<grpcProtobufMessages::SubscribeStreamMessage>()));
    IProvider* provider = grpc_provider.get();
    provider->start();
    provider->subscribe("Vehicle.Temp");

    // Give a moment for the subscription thread to start and process the response
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Now, expect the unsubscribe RPC call.
    EXPECT_CALL(*mock_stub_ptr, UnsubscribeRequest(_, ::testing::Property(&grpcProtobufMessages::UnsubscribeRequestMessage::subscriptionid, "sub-to-unsub"), _))
        .Times(1)
        .WillOnce(Return(grpc::Status::OK));

    // 3. Call unsubscribe.
    grpc_provider->unsubscribe("Vehicle.Temp");

    // 4. Give a moment for the unsubscribe to be processed.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(GrpcProviderTest, UnsubscribeWithInvalidPath) {
    IProvider* provider = grpc_provider.get();
    ASSERT_NO_THROW(provider->unsubscribe("Vehicle.Temp"));
}
