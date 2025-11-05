#ifndef GRPC_TRANSPORT_H
#define GRPC_TRANSPORT_H

#include <memory>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "ITransport.h"
#include "VISSv3.0.grpc.pb.h"
#include "ObjectPool.h"
#include <atomic>

// To switch between sync and async server, define/undefine this macro
//#define USE_ASYNC_GRPC_SERVER

#ifdef USE_ASYNC_GRPC_SERVER
#include "VISSAsyncServiceImpl.h"
#endif

// Forward declarations
class VssSubscriptionManager;

// The gRPC service implementation
class VISSServiceImpl final : public grpcProtobufMessages::VISS::Service {
public:
    explicit VISSServiceImpl(
        std::shared_ptr<ObjectPool<RequestData>> pool,
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue
    );

    grpc::Status GetRequest(grpc::ServerContext* context, const grpcProtobufMessages::GetRequestMessage* request, grpcProtobufMessages::GetResponseMessage* response) override;
    grpc::Status SetRequest(grpc::ServerContext* context, const grpcProtobufMessages::SetRequestMessage* request, grpcProtobufMessages::SetResponseMessage* response) override;
    grpc::Status SubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::SubscribeRequestMessage* request, grpc::ServerWriter<grpcProtobufMessages::SubscribeStreamMessage>* writer) override;
    grpc::Status UnsubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::UnsubscribeRequestMessage* request, grpcProtobufMessages::UnsubscribeResponseMessage* response) override;

private:
    std::shared_ptr<ObjectPool<RequestData>> pool_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue;
};

// The GrpcTransport class that manages the gRPC server lifecycle
class GrpcTransport : public ITransport {
public:
    explicit GrpcTransport(
        std::shared_ptr<ObjectPool<RequestData>> pool,
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue
    );
    ~GrpcTransport();

    void start() override;
    void stop() override;

private:
#ifdef USE_ASYNC_GRPC_SERVER
    std::string server_address;
    std::unique_ptr<VISSAsyncServiceImpl> async_service_;
    std::thread server_thread_;
#else
    std::string server_address;
    std::unique_ptr<VISSServiceImpl> service;
    std::unique_ptr<grpc::Server> server;
    std::thread server_thread_;
    std::atomic<bool> m_stopped;
    //std::unique_ptr<grpc::ServerCompletionQueue> cq;
#endif
};

#endif // GRPC_TRANSPORT_H
