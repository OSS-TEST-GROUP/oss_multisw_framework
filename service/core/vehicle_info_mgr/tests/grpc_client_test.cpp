#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpcProtobufMessages::VISS;
using grpcProtobufMessages::GetRequestMessage;
using grpcProtobufMessages::GetResponseMessage;
using grpcProtobufMessages::SetRequestMessage;
using grpcProtobufMessages::SetResponseMessage;
using grpcProtobufMessages::SubscribeRequestMessage;
using grpcProtobufMessages::SubscribeStreamMessage;
using grpcProtobufMessages::UnsubscribeRequestMessage;
using grpcProtobufMessages::UnsubscribeResponseMessage;

class VissClient {
public:
    VissClient(std::shared_ptr<Channel> channel)
        : stub_(grpcProtobufMessages::VISS::NewStub(channel)) {}

    std::string Get(const std::string& path) {
        GetRequestMessage request;
        request.set_path(path);
        request.set_requestid("get-test-1");

        GetResponseMessage response;
        ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

        Status status = stub_->GetRequest(&context, request, &response);

        if (status.ok()) {
            if (response.status() == grpcProtobufMessages::ResponseStatus::SUCCESS) {
                if (response.has_successresponse() && response.successresponse().has_datapack() && response.successresponse().datapack().data_size() > 0 && response.successresponse().datapack().data(0).dp_size() > 0) {
                    return response.successresponse().datapack().data(0).dp(0).value();
                } else {
                    return "[SUCCESS but no data]";
                }
            } else {
                return "ERROR: " + response.errorresponse().reason();
            }
        } else {
            return "RPC failed: " + status.error_message();
        }
    }

    std::string Set(const std::string& path, const std::string& value) {
        SetRequestMessage request;
        request.set_path(path);
        request.set_value(value);
        request.set_requestid("set-test-1");

        SetResponseMessage response;
        ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

        Status status = stub_->SetRequest(&context, request, &response);

        if (status.ok()) {
            if (response.status() == grpcProtobufMessages::ResponseStatus::SUCCESS) {
                return "SUCCESS";
            } else {
                return "ERROR: " + response.errorresponse().reason();
            }
        } else {
            return "RPC failed: " + status.error_message();
        }
    }

    std::string Subscribe(const std::string& path, std::atomic<bool>& finish_flag) {
        SubscribeRequestMessage request;
        request.set_path(path);
        request.set_requestid("sub-test-1");

        SubscribeStreamMessage response;
        ClientContext context;

        auto reader = stub_->SubscribeRequest(&context, request);

        if (!reader->Read(&response)) {
            return "RPC failed or stream ended prematurely.";
        }

        if (response.status() != grpcProtobufMessages::ResponseStatus::SUCCESS) {
            return "Subscription rejected: " + response.response().errorresponse().reason();
        }
        
        std::string subId = response.response().subscriptionid();
        std::cout << "Subscription successful. Subscription ID: " << subId << std::endl;

        while (finish_flag.load() == false) {
            if (reader->Read(&response)) {
                std::cout << "Received event: " 
                          << response.event().successresponse().datapack().data(0).dp(0).value() 
                          << std::endl;
            } else {
                break;
            }
        }
        
        context.TryCancel();
        reader->Finish();
        return subId;
    }

    std::string Unsubscribe(const std::string& subscriptionId) {
        UnsubscribeRequestMessage request;
        request.set_subscriptionid(subscriptionId);
        request.set_requestid("unsub-test-1");

        UnsubscribeResponseMessage response;
        ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

        Status status = stub_->UnsubscribeRequest(&context, request, &response);

        if (status.ok()) {
            if (response.status() == grpcProtobufMessages::ResponseStatus::SUCCESS) {
                return "SUCCESS";
            } else {
                return "ERROR: " + response.errorresponse().reason();
            }
        } else {
            return "RPC failed: " + status.error_message();
        }
    }

private:
    std::unique_ptr<VISS::Stub> stub_;
};

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " --action <get|set|subscribe> --path <vss_path> [options]\n\n"
              << "Options:\n"
              << "  --help                         Show this help message.\n"
              << "  --server <address>             Set the server address. (Default: localhost:50051)\n"
              << "  --action <get|set|subscribe>   The action to perform.\n"
              << "  --path <vss_path>              The VSS path for the action.\n"
              << "  --value <value>                The value for the 'set' action.\n"
              << std::endl;
}

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    std::string action;
    std::string path;
    std::string value;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--server" && i + 1 < argc) {
            target_str = argv[++i];
        } else if (arg == "--action" && i + 1 < argc) {
            action = argv[++i];
        } else if (arg == "--path" && i + 1 < argc) {
            path = argv[++i];
        } else if (arg == "--value" && i + 1 < argc) {
            value = argv[++i];
        }
    }

    if (action.empty() || path.empty()) {
        std::cerr << "Error: --action and --path are required." << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    std::cout << "Connecting to server at " << target_str << std::endl;
    auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    if (!channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(5))) {
        std::cerr << "Error: Client failed to connect to server." << std::endl;
        return 1;
    }
    std::cout << "Client connected to server successfully." << std::endl;
    VissClient client(channel);

    if (action == "get") {
        std::cout << "--- Performing GET request for path: " << path << " ---" << std::endl;
        std::string response = client.Get(path);
        std::cout << "Server response: " << response << std::endl;
    } else if (action == "set") {
        if (value.empty()) {
            std::cerr << "Error: --value is required for 'set' action." << std::endl;
            return 1;
        }
        std::cout << "--- Performing SET request for path: " << path << " with value: " << value << " ---" << std::endl;
        std::string response = client.Set(path, value);
        std::cout << "Server response: " << response << std::endl;
    } else if (action == "subscribe") {
        std::cout << "--- Performing SUBSCRIBE request for path: " << path << " ---" << std::endl;
        std::cout << "Press Ctrl+C to stop subscribing." << std::endl;
        
        std::atomic<bool> finish_flag(false);
        std::string subscriptionId = client.Subscribe(path, finish_flag);

        // The subscribe call is blocking in this test implementation, 
        // so we don't need to do anything else here. The user can Ctrl+C to exit.
        // A real implementation might handle unsubscription more gracefully.
    } else {
        std::cerr << "Error: Invalid action '" << action << "'. Must be one of: get, set, subscribe." << std::endl;
        return 1;
    }

    return 0;
}
