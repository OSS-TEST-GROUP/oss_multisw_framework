#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"

// Simple logger for the test server
#define LOG(level) std::cout << "[" << #level << "] "

class TestVISSServiceImpl final : public grpcProtobufMessages::VISS::Service {
public:
    grpc::Status GetRequest(grpc::ServerContext* context, const grpcProtobufMessages::GetRequestMessage* request, grpcProtobufMessages::GetResponseMessage* response) override {
        LOG(INFO) << "Received GetRequest for path: " << request->path() << std::endl;
        
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        auto* success_resp = response->mutable_successresponse();
        auto* data_pack = success_resp->mutable_datapack();
        auto* data = data_pack->add_data();
        data->set_path(request->path());
        auto* dp = data->add_dp();
        dp->set_value("123"); // Dummy value
        dp->set_ts("2025-10-23T12:00:00Z");

        return grpc::Status::OK;
    }

    grpc::Status SetRequest(grpc::ServerContext* context, const grpcProtobufMessages::SetRequestMessage* request, grpcProtobufMessages::SetResponseMessage* response) override {
        LOG(INFO) << "Received SetRequest for path: " << request->path() << " with value: " << request->value() << std::endl;
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        return grpc::Status::OK;
    }

    grpc::Status SubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::SubscribeRequestMessage* request, grpc::ServerWriter<grpcProtobufMessages::SubscribeStreamMessage>* writer) override {
        LOG(INFO) << "Received SubscribeRequest for path: " << request->path() << std::endl;

        // 1. Send subscription success response
        grpcProtobufMessages::SubscribeStreamMessage initial_response;
        initial_response.set_mtype(grpcProtobufMessages::SubscribeResponseType::RESPONSE);
        initial_response.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        std::string sub_id = "sub-" + request->path();
        initial_response.mutable_response()->set_subscriptionid(sub_id);
        writer->Write(initial_response);

        // 2. Periodically send dummy events
        for (int i = 0; i < 1000; ++i) {
            if (context->IsCancelled()) {
                LOG(INFO) << "Context canceled." << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));

            grpcProtobufMessages::SubscribeStreamMessage event_msg;
            event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
            event_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
            auto* event = event_msg.mutable_event();
            event->set_subscriptionid(sub_id);
            auto* data_pack = event->mutable_successresponse()->mutable_datapack();
            auto* data = data_pack->add_data();
            data->set_path("Vehicle.Speed"); // Dummy path
            auto* dp = data->add_dp();
            dp->set_value(std::to_string(100 + i));
            dp->set_ts("2025-10-23T12:00:0" + std::to_string(i) + "Z");
            
            LOG(INFO) << "Sending event for " << sub_id << ": Vehicle.Speed = " << 100 + i << std::endl;
            writer->Write(event_msg);
        }

        LOG(INFO) << "Finished sending events for " << sub_id << std::endl;
        return grpc::Status::OK;
    }

    grpc::Status UnsubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::UnsubscribeRequestMessage* request, grpcProtobufMessages::UnsubscribeResponseMessage* response) override {
        LOG(INFO) << "Received UnsubscribeRequest for subscriptionId: " << request->subscriptionid() << std::endl;
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        return grpc::Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50052");
    TestVISSServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG(INFO) << "Test gRPC server listening on " << server_address << std::endl;

    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
