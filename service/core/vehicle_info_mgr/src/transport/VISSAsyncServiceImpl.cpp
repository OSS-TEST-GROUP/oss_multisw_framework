#include "VISSAsyncServiceImpl.h"
#include "Logger.h"
#include "JsonSchemaValidator.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

// Helper function to populate a DataPackage from a JSON object

void populateDataPackagess(const rapidjson::Value& data_item, grpcProtobufMessages::DataPackages::DataPackage* data_package) {
    if (!data_item.IsObject()) return;

    if (data_item.HasMember("path") && data_item["path"].IsString()) {
        data_package->set_path(data_item["path"].GetString());
    }

    if (data_item.HasMember("dp") && data_item["dp"].IsObject()) {
        const auto& dp_obj = data_item["dp"];
        auto dp = data_package->add_dp();
        if (dp_obj.HasMember("value") && dp_obj["value"].IsString()) {
            dp->set_value(dp_obj["value"].GetString());
        }
        if (dp_obj.HasMember("ts") && dp_obj["ts"].IsString()) {
            dp->set_ts(dp_obj["ts"].GetString());
        }
    }
}

class GetCallData : public VISSAsyncServiceImpl::CallData {
public:
    GetCallData(grpcProtobufMessages::VISS::AsyncService* service, grpc::ServerCompletionQueue* cq, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue, std::shared_ptr<ObjectPool<RequestData>> pool)
        : requestQueue_(requestQueue), pool_(pool), responder_(&ctx_) {
        service_ = service;
        cq_ = cq;
        status_ = CREATE;
        Proceed();
    }

    void Proceed(bool ok = true) override {
        if (status_ == CREATE) {
            status_ = PROCESS;
            service_->RequestGetRequest(&ctx_, &request_, &responder_, cq_, cq_, this);
        } else if (status_ == PROCESS) {
            new GetCallData(service_, cq_, requestQueue_, pool_);
            if (!ok) { delete this; return; }

            auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
            
            rapidjson::Document temp_doc;
            temp_doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = temp_doc.GetAllocator();
            temp_doc.AddMember("action", "get", allocator);
            temp_doc.AddMember("path", rapidjson::Value(request_.path().c_str(), allocator), allocator);
            temp_doc.AddMember("requestId", rapidjson::Value(request_.requestid().c_str(), allocator), allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            temp_doc.Accept(writer);

            auto requestData = pool_->acquire();
            requestData->requestId = request_.requestid();
            requestData->responseChan = responseChan;
            requestData->request_doc->Parse(buffer.GetString());
            
            requestQueue_->push(requestData);

            std::shared_ptr<PooledResponseMap> response_doc;
            responseChan->wait_and_pop(response_doc);

            if (response_doc->HasMember("error")) {
                response_.set_status(grpcProtobufMessages::ResponseStatus::ERROR);
            } else {
                response_.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
            }

            status_ = FINISH;
            responder_.Finish(response_, grpc::Status::OK, this);
        } else {
            delete this;
        }
    }

private:
    grpcProtobufMessages::GetRequestMessage request_;
    grpcProtobufMessages::GetResponseMessage response_;
    grpc::ServerAsyncResponseWriter<grpcProtobufMessages::GetResponseMessage> responder_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue_;
    std::shared_ptr<ObjectPool<RequestData>> pool_;
};

class SetCallData : public VISSAsyncServiceImpl::CallData {
public:
    SetCallData(grpcProtobufMessages::VISS::AsyncService* service, grpc::ServerCompletionQueue* cq, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue, std::shared_ptr<ObjectPool<RequestData>> pool)
        : requestQueue_(requestQueue), pool_(pool), responder_(&ctx_) {
        service_ = service;
        cq_ = cq;
        status_ = CREATE;
        Proceed();
    }

    void Proceed(bool ok = true) override {
        if (status_ == CREATE) {
            status_ = PROCESS;
            service_->RequestSetRequest(&ctx_, &request_, &responder_, cq_, cq_, this);
        } else if (status_ == PROCESS) {
            new SetCallData(service_, cq_, requestQueue_, pool_);
            if (!ok) { delete this; return; }

            auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
            
            rapidjson::Document temp_doc;
            temp_doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = temp_doc.GetAllocator();
            temp_doc.AddMember("action", "set", allocator);
            temp_doc.AddMember("path", rapidjson::Value(request_.path().c_str(), allocator), allocator);
            temp_doc.AddMember("value", rapidjson::Value(request_.value().c_str(), allocator), allocator);
            temp_doc.AddMember("requestId", rapidjson::Value(request_.requestid().c_str(), allocator), allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            temp_doc.Accept(writer);

            auto requestData = pool_->acquire();
            requestData->requestId = request_.requestid();
            requestData->responseChan = responseChan;
            requestData->request_doc->Parse(buffer.GetString());
            
            requestQueue_->push(requestData);

            std::shared_ptr<PooledResponseMap> response_doc;
            responseChan->wait_and_pop(response_doc);

            if (response_doc->HasMember("error")) {
                response_.set_status(grpcProtobufMessages::ResponseStatus::ERROR);
            } else {
                response_.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
            }

            status_ = FINISH;
            responder_.Finish(response_, grpc::Status::OK, this);
        } else {
            delete this;
        }
    }

private:
    grpcProtobufMessages::SetRequestMessage request_;
    grpcProtobufMessages::SetResponseMessage response_;
    grpc::ServerAsyncResponseWriter<grpcProtobufMessages::SetResponseMessage> responder_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue_;
    std::shared_ptr<ObjectPool<RequestData>> pool_;
};

class SubscribeCallData : public VISSAsyncServiceImpl::CallData {
public:
    SubscribeCallData(VISSAsyncServiceImpl* serviceImpl, grpcProtobufMessages::VISS::AsyncService* service, grpc::ServerCompletionQueue* cq, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue, std::shared_ptr<ObjectPool<RequestData>> pool)
        : service_impl_(serviceImpl), requestQueue_(requestQueue), pool_(pool), responder_(&ctx_), is_cancelled_(false) {
        service_ = service;
        cq_ = cq;
        status_ = CREATE;
        Proceed();
    }

    ~SubscribeCallData() {
        if (streaming_thread_.joinable()) {
            streaming_thread_.join();
        }
        if (!subscription_id_.empty()) {
            service_impl_->removeSubscription(subscription_id_);
        }
    }

    void cancel() {
        is_cancelled_ = true;
    }

    void invalidateResponseChannel() {
        if (responseChan_) {
            responseChan_->invalidate();
        }
    }

    void Proceed(bool ok = true) override {
        if (status_ == CREATE) {
            status_ = PROCESS;
            service_->RequestSubscribeRequest(&ctx_, &request_, &responder_, cq_, cq_, this);
        } else if (status_ == PROCESS) {
            new SubscribeCallData(service_impl_, service_, cq_, requestQueue_, pool_);
            if (!ok) {
                delete this;
                return;
            }

            responseChan_ = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
            
            rapidjson::Document temp_doc;
            temp_doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = temp_doc.GetAllocator();

            temp_doc.AddMember("action", "subscribe", allocator);
            temp_doc.AddMember("path", rapidjson::Value(request_.path().c_str(), allocator), allocator);
            temp_doc.AddMember("requestId", rapidjson::Value(request_.requestid().c_str(), allocator), allocator);
            
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            temp_doc.Accept(writer);

            auto subscribeRequest = pool_->acquire();
            subscribeRequest->requestId = request_.requestid();
            subscribeRequest->responseChan = responseChan_;
            subscribeRequest->request_doc->Parse(buffer.GetString());
            
            requestQueue_->push(subscribeRequest);

            std::shared_ptr<PooledResponseMap> initial_response_doc;
            responseChan_->wait_and_pop(initial_response_doc);

            grpcProtobufMessages::SubscribeStreamMessage initial_response;
            if (initial_response_doc && initial_response_doc->HasMember("subscriptionId") && (*initial_response_doc)["subscriptionId"].IsString()) {
                subscription_id_ = (*initial_response_doc)["subscriptionId"].GetString();
            }

            initial_response.set_mtype(grpcProtobufMessages::SubscribeResponseType::RESPONSE);
            auto resp_msg = initial_response.mutable_response();
            if (initial_response_doc && initial_response_doc->HasMember("error") && (*initial_response_doc)["error"].IsObject()) {
                initial_response.set_status(grpcProtobufMessages::ResponseStatus::ERROR);
            } else {
                initial_response.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
                resp_msg->set_subscriptionid(subscription_id_);
                service_impl_->addSubscription(subscription_id_, this);
            }
            resp_msg->set_requestid(request_.requestid());
            
            status_ = STREAMING;
            responder_.Write(initial_response, this);
            
            // Start the streaming thread only after the initial write is requested
            streaming_thread_ = std::thread([this]() {
                std::shared_ptr<PooledResponseMap> event_doc;
                while (!is_cancelled_ && responseChan_->wait_and_pop(event_doc)) {
                    if (!event_doc) break;

                    grpcProtobufMessages::SubscribeStreamMessage event_msg;
                    event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
                    event_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
                    auto event = event_msg.mutable_event();
                    event->set_subscriptionid(subscription_id_);
                    if (event_doc->HasMember("data")) {
                        auto success_resp = event->mutable_successresponse();
                        auto data_pack = success_resp->mutable_datapack();
                        const auto& data_val = (*event_doc)["data"];
                        if (data_val.IsArray()) {
                            for (const auto& data_item : data_val.GetArray()) {
                                populateDataPackagess(data_item, data_pack->add_data());
                            }
                        } else if (data_val.IsObject()) {
                            populateDataPackagess(data_val, data_pack->add_data());
                        }
                    }
                    responder_.Write(event_msg, this);
                }
                // The loop is finished. Request to finish the RPC.
                status_ = FINISH_REQUESTED;
                void* tag = this;
                bool ignored_ok;
                cq_->AsyncNext(&tag, &ignored_ok, gpr_time_0(GPR_CLOCK_MONOTONIC));
            });

        } else if (status_ == STREAMING && ok) {
            // A Write operation has completed. The streaming thread is waiting for the next data.
            // We do nothing here and just wait for the next event.
            return;
        } else if (status_ == FINISH_REQUESTED) {
            status_ = FINISH;
            responder_.Finish(grpc::Status::OK, this);
        } else if (status_ == FINISH) {
            delete this;
        } else { // This 'else' block is for cancellation (ok=false)
            is_cancelled_ = true;
            if (responseChan_) {
                responseChan_->invalidate();
            }
            if (status_ != FINISH) {
                status_ = FINISH;
                responder_.Finish(grpc::Status::CANCELLED, this);
            } else {
                delete this;
            }
        }
    }
private:
    VISSAsyncServiceImpl* service_impl_;
    grpcProtobufMessages::SubscribeRequestMessage request_;
    grpc::ServerAsyncWriter<grpcProtobufMessages::SubscribeStreamMessage> responder_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue_;
    std::shared_ptr<ObjectPool<RequestData>> pool_;
    
    std::thread streaming_thread_;
    std::atomic<bool> is_cancelled_;
    std::string subscription_id_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>> responseChan_;
};

class UnsubscribeCallData : public VISSAsyncServiceImpl::CallData {
public:
    UnsubscribeCallData(VISSAsyncServiceImpl* serviceImpl, grpcProtobufMessages::VISS::AsyncService* service, grpc::ServerCompletionQueue* cq, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue, std::shared_ptr<ObjectPool<RequestData>> pool)
        : service_impl_(serviceImpl), requestQueue_(requestQueue), pool_(pool), responder_(&ctx_) {
        service_ = service;
        cq_ = cq;
        status_ = CREATE;
        Proceed();
    }

    void Proceed(bool ok = true) override {
        if (status_ == CREATE) {
            status_ = PROCESS;
            service_->RequestUnsubscribeRequest(&ctx_, &request_, &responder_, cq_, cq_, this);
        } else if (status_ == PROCESS) {
            new UnsubscribeCallData(service_impl_, service_, cq_, requestQueue_, pool_);
            if (!ok) { delete this; return; }

            auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
            
            auto unsubscribeRequest = pool_->acquire();
            // For unsubscribe, the requestId from the core logic's perspective is the subscriptionId.
            unsubscribeRequest->requestId = request_.subscriptionid();
            unsubscribeRequest->responseChan = responseChan;

            // Find the subscription and signal it to cancel
            auto* subCallData = static_cast<SubscribeCallData*>(service_impl_->findSubscription(request_.subscriptionid()));
            if (subCallData) {
                subCallData->cancel();
                // Also invalidate the queue to unblock the streaming thread
                subCallData->invalidateResponseChannel();
            }

            unsubscribeRequest->request_doc->SetObject();
            rapidjson::Document::AllocatorType& allocator = unsubscribeRequest->request_doc->GetAllocator();
            rapidjson::Value action_val("unsubscribe", allocator);
            rapidjson::Value sub_id_val(request_.subscriptionid().c_str(), allocator);

            unsubscribeRequest->request_doc->AddMember("action", action_val, allocator);
            unsubscribeRequest->request_doc->AddMember("subscriptionId", sub_id_val, allocator);
            
            requestQueue_->push(unsubscribeRequest);

            std::shared_ptr<PooledResponseMap> response_doc;
            responseChan->wait_and_pop(response_doc);
            
            response_.set_requestid(request_.requestid());
            if (response_doc && response_doc->HasMember("error") && (*response_doc)["error"].IsObject()) {
                response_.set_status(grpcProtobufMessages::ResponseStatus::ERROR);
            } else {
                response_.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
            }

            status_ = FINISH;
            responder_.Finish(response_, grpc::Status::OK, this);
        } else {
            delete this;
        }
    }
private:
    VISSAsyncServiceImpl* service_impl_;
    grpcProtobufMessages::UnsubscribeRequestMessage request_;
    grpcProtobufMessages::UnsubscribeResponseMessage response_;
    grpc::ServerAsyncResponseWriter<grpcProtobufMessages::UnsubscribeResponseMessage> responder_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue_;
    std::shared_ptr<ObjectPool<RequestData>> pool_;
};


VISSAsyncServiceImpl::VISSAsyncServiceImpl(
    std::shared_ptr<ObjectPool<RequestData>> pool,
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue)
    : pool_(pool), requestQueue_(queue) {}

VISSAsyncServiceImpl::~VISSAsyncServiceImpl() {
    Shutdown();
}

void VISSAsyncServiceImpl::Run(const std::string& server_address) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    LOG_INFO << "Async gRPC server listening on " << server_address;

    // Initial request handlers for all RPCs
    new GetCallData(&service_, cq_.get(), requestQueue_, pool_);
    new SetCallData(&service_, cq_.get(), requestQueue_, pool_);
    new SubscribeCallData(this, &service_, cq_.get(), requestQueue_, pool_);
    new UnsubscribeCallData(this, &service_, cq_.get(), requestQueue_, pool_);

    int num_threads = std::thread::hardware_concurrency();
    for (int i = 0; i < num_threads; i++) {
        worker_threads_.emplace_back(&VISSAsyncServiceImpl::HandleRpcs, this);
    }
    LOG_INFO << "Async gRPC server started with " << num_threads << " worker threads.";

    for(auto& thread : worker_threads_) {
        thread.join();
    }
}

void VISSAsyncServiceImpl::Shutdown() {
    if (server_) {
        // 1. Stop accepting new RPCs.
        server_->Shutdown();
    }

    if (cq_) {
        // 2. Shut down the completion queue. This will cause cq_->Next() to start returning false.
        cq_->Shutdown();
    }

    // 3. Join the worker threads. They will exit the HandleRpcs loop 
    //    once the queue is drained and cq_->Next() returns false.
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // It's also a good practice to drain the queue of any final events after shutdown.
    // The tags that come out of the queue after shutdown are the CallData objects
    // that were still in flight. They must be deleted.
    LOG_INFO << "Draining completion queue...";
    void* tag;
    bool ok;
    while (cq_ && cq_->Next(&tag, &ok)) {
        LOG_INFO << "Draining and deleting tag: " << tag;
        delete static_cast<CallData*>(tag);
    }
    LOG_INFO << "Completion queue drained.";
}

void VISSAsyncServiceImpl::HandleRpcs() {
    void* tag;
    bool ok;
    while (cq_->Next(&tag, &ok)) {
        if (ok) {
            static_cast<CallData*>(tag)->Proceed();
        }
    }
}

void VISSAsyncServiceImpl::addSubscription(const std::string& subId, CallData* callData) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    active_subscriptions_[subId] = callData;
}

void VISSAsyncServiceImpl::removeSubscription(const std::string& subId) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    active_subscriptions_.erase(subId);
}

VISSAsyncServiceImpl::CallData* VISSAsyncServiceImpl::findSubscription(const std::string& subId) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = active_subscriptions_.find(subId);
    if (it != active_subscriptions_.end()) {
        return it->second;
    }
    return nullptr;
}
