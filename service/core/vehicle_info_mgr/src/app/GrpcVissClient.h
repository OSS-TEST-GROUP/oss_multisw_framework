#ifndef GRPC_VISS_CLIENT_H
#define GRPC_VISS_CLIENT_H

#include "IVissClient.h"
#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"
#include <map>
#include <mutex>
#include <atomic>
#include <vector>
#include <thread>

class GrpcVissClient : public IVissClient {
public:
    GrpcVissClient();
    virtual ~GrpcVissClient();

    void start(const std::string& address) override;
    void stop() override;
    void sendRequest(const std::string& jsonReq) override;

    void Get(const std::string& path, const std::string& jsonFilter = "", const std::string& reqId = "");
    void Set(const std::string& path, const std::string& value, const std::string& reqId = "");
    void Subscribe(const std::string& path, const std::string& jsonFilter = "", const std::string& reqId = "");
    void Unsubscribe(const std::string& subId, const std::string& reqId = "");

private:
    void parseFilter(const std::string& jsonFilter, grpcProtobufMessages::FilterExpressions* filterExprs);
    struct SubscriptionInfo {
        std::thread thread;
        std::shared_ptr<grpc::ClientContext> context;
        std::atomic<bool> stopFlag;
        SubscriptionInfo() : stopFlag(false) {}
    };

    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<grpcProtobufMessages::VISS::Stub> m_stub;
    
    std::map<std::string, std::shared_ptr<SubscriptionInfo>> m_subscriptions;
    std::mutex m_subMutex;
};

#endif // GRPC_VISS_CLIENT_H
