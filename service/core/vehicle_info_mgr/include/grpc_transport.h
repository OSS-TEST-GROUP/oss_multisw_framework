#ifndef GRPC_TRANSPORT_H
#define GRPC_TRANSPORT_H

#include <memory>
#include <grpcpp/grpcpp.h>
#include "ITransport.h"
#include "VISSv3.0.grpc.pb.h"
#include "ObjectPool.h"

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
    std::string server_address;
    std::unique_ptr<VISSServiceImpl> service;
    std::unique_ptr<grpc::Server> server;
};

#endif // GRPC_TRANSPORT_H
