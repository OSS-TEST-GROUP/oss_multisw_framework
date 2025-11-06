#ifndef VISS_ASYNC_SERVICE_IMPL_H
#define VISS_ASYNC_SERVICE_IMPL_H

#include <memory>
#include <thread>
#include <vector>
#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"
#include "ITransport.h" // Includes ThreadSafeQueue, ObjectPool, Types, and RequestData
#include "ObjectPool.h"

class VISSAsyncServiceImpl {
    friend class SubscribeCallData;
    friend class UnsubscribeCallData;
public:
    VISSAsyncServiceImpl(
        std::shared_ptr<ObjectPool<RequestData>> pool,
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue
    );
    ~VISSAsyncServiceImpl();

    void Run(const std::string& server_address);
    void Shutdown();

public:
    class CallData {
    public:
        virtual ~CallData() = default;
        virtual void Proceed(bool ok = true) = 0;
    protected:
        grpcProtobufMessages::VISS::AsyncService* service_;
        grpc::ServerCompletionQueue* cq_;
        grpc::ServerContext ctx_;
        enum CallStatus { CREATE, PROCESS, STREAMING, FINISH_REQUESTED, FINISH };
        CallStatus status_; 
    };

    void addSubscription(const std::string& subId, CallData* callData);
    void removeSubscription(const std::string& subId);
    CallData* findSubscription(const std::string& subId);

private:

    void HandleRpcs();

    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    grpcProtobufMessages::VISS::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;
    
    std::shared_ptr<ObjectPool<RequestData>> pool_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue_;
    std::vector<std::thread> worker_threads_;

    // For managing active subscriptions
    std::mutex subscriptions_mutex_;
    std::map<std::string, CallData*> active_subscriptions_;
};

#endif // VISS_ASYNC_SERVICE_IMPL_H
