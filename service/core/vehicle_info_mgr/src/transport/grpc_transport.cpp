#include <iostream>
#include <google/protobuf/util/json_util.h>
#include "grpc_transport.h"
#include "VissServerApp.h" // For VssSubscriptionManager
#include "Logger.h"
#include "Utils.h"
#include "JsonSchemaValidator.h"
#include "ConfigManager.h"

// Helper function to populate an ErrorMessage from a JSON object
void populateErrorResponse(const rapidjson::Value& error_val, grpcProtobufMessages::ErrorResponseMessage* error_resp) {
    const rapidjson::Value* error_obj_ptr = nullptr;

    if (error_val.IsObject()) {
        error_obj_ptr = &error_val;
    } else if (error_val.IsArray() && !error_val.Empty()) {
        error_obj_ptr = &error_val[0];
    }

    if (error_obj_ptr) {
        const auto& error_obj = *error_obj_ptr;
        if (error_obj.HasMember("number") && error_obj["number"].IsString()) {
            error_resp->set_number(error_obj["number"].GetString());
        }
        if (error_obj.HasMember("reason") && error_obj["reason"].IsString()) {
            error_resp->set_reason(error_obj["reason"].GetString());
        }
        if (error_obj.HasMember("description") && error_obj["description"].IsString()) {
            error_resp->set_description(error_obj["description"].GetString());
        }
    }
}

static std::string extractValueAsString(const rapidjson::Value& val) {
    if (val.IsString()) {
        return val.GetString();
    }
    // Object, Array, Number, Bool 등 → JSON 문자열로 직렬화
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> w(buf);
    val.Accept(w);
    return buf.GetString();
}

void populateDataPackage(const rapidjson::Value& data_item, grpcProtobufMessages::DataPackages::DataPackage* data_package) {
    //if (!data_item.IsObject()) return;

    if (data_item.HasMember("path") && data_item["path"].IsString()) {
        data_package->set_path(data_item["path"].GetString());
    }

    if (data_item.HasMember("dp")) {
        const auto& dp_val = data_item["dp"];
        if (dp_val.IsArray()) {
            for (const auto& dp_item : dp_val.GetArray()) {
                if (dp_item.IsObject()) {
                    const auto& dp_obj = dp_item;
                    auto dp = data_package->add_dp();
                    if (dp_obj.HasMember("value")) {
                        std::string value_str = extractValueAsString(dp_obj["value"]);
                        dp->set_value(value_str);
                    }
                    if (dp_obj.HasMember("ts") && dp_obj["ts"].IsString()) {
                        dp->set_ts(dp_obj["ts"].GetString());
                    }
                }
            }
        } else {
            const auto& dp_obj = dp_val;
            auto dp = data_package->add_dp();
            if (dp_obj.HasMember("value")) {
                std::string value_str = extractValueAsString(dp_obj["value"]);
                dp->set_value(value_str);
            }
            if (dp_obj.HasMember("ts") && dp_obj["ts"].IsString()) {
                dp->set_ts(dp_obj["ts"].GetString());
            }
        }
    }
}

// Helper function to convert a single FilterExpression protobuf to a rapidjson Value
rapidjson::Value convertSingleFilter(const grpcProtobufMessages::FilterExpressions::FilterExpression& filter_exp, rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value exp_obj(rapidjson::kObjectType);
    rapidjson::Value param_array(rapidjson::kArrayType);
    bool has_parameter = false;

    std::string variant_str;
    switch (filter_exp.variant()) {
        case grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_PATHS:
            variant_str = "paths";
            if (filter_exp.value().has_valuepaths()) {
                for (const auto& p : filter_exp.value().valuepaths().relativepath()) {
                    param_array.PushBack(rapidjson::Value(p.c_str(), allocator), allocator);
                }
                has_parameter = true;
            }
            break;
        case grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_TIMEBASED:
            variant_str = "timebased";
            if (filter_exp.value().has_valuetimebased()) {
                rapidjson::Value param_obj(rapidjson::kObjectType);
                param_obj.AddMember("period", rapidjson::Value(filter_exp.value().valuetimebased().period().c_str(), allocator), allocator);
                param_array.PushBack(param_obj, allocator);
                has_parameter = true;
            }
            break;
        case grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_RANGE:
            variant_str = "range";
            if (filter_exp.value().valuerange_size() > 0) {
                for (const auto& range : filter_exp.value().valuerange()) {
                    rapidjson::Value range_obj(rapidjson::kObjectType);
                    range_obj.AddMember("logic-op", rapidjson::Value(range.logicoperator().c_str(), allocator), allocator);
                    range_obj.AddMember("boundary", rapidjson::Value(range.boundary().c_str(), allocator), allocator);
                    param_array.PushBack(range_obj, allocator);
                }
                has_parameter = true;
            }
            break;
        case grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_CHANGE:
            variant_str = "change";
            if (filter_exp.value().has_valuechange()) {
                rapidjson::Value param_obj(rapidjson::kObjectType);
                param_obj.AddMember("logic-op", rapidjson::Value(filter_exp.value().valuechange().logicoperator().c_str(), allocator), allocator);
                param_obj.AddMember("diff", rapidjson::Value(filter_exp.value().valuechange().diff().c_str(), allocator), allocator);
                param_array.PushBack(param_obj, allocator);
                has_parameter = true;
            }
            break;
        case grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_HISTORY:
            variant_str = "history";
            if (filter_exp.value().has_valuehistory()) {
                param_array.PushBack(rapidjson::Value(filter_exp.value().valuehistory().timeperiod().c_str(), allocator), allocator);
                has_parameter = true;
            }
            break;
        case grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_METADATA:
            variant_str = "metadata";
            if (filter_exp.value().has_valuemetadata()) {
                param_array.PushBack(rapidjson::Value(filter_exp.value().valuemetadata().tree().c_str(), allocator), allocator);
                has_parameter = true;
            }
            break;
        default:
            return rapidjson::Value(rapidjson::kNullType); // Return null for unknown variants
    }
    exp_obj.AddMember("variant", rapidjson::Value(variant_str.c_str(), allocator), allocator);
    if (has_parameter) {
        exp_obj.AddMember("parameter", param_array, allocator);
    }
    return exp_obj;
}

// Helper function to convert protobuf FilterExpressions to rapidjson Value
void populateFilterExpressions(const grpcProtobufMessages::FilterExpressions& filters, rapidjson::Value& out_value, rapidjson::Document::AllocatorType& allocator) {
    out_value.SetArray();
    for (const auto& filter_exp : filters.filterexp()) {
        rapidjson::Value exp_obj = convertSingleFilter(filter_exp, allocator);
        if (!exp_obj.IsNull()) {
            out_value.PushBack(exp_obj, allocator);
        }
    }
}


// VISSServiceImpl implementation
VISSServiceImpl::VISSServiceImpl(
    std::shared_ptr<ObjectPool<RequestData>> pool,
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue)
    : pool_(pool), requestQueue(queue) {}

grpc::Status VISSServiceImpl::GetRequest(grpc::ServerContext* context, const grpcProtobufMessages::GetRequestMessage* request, grpcProtobufMessages::GetResponseMessage* response) {
    auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    
    rapidjson::Document temp_doc;
    temp_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = temp_doc.GetAllocator();

    temp_doc.AddMember("action", "get", allocator);
    temp_doc.AddMember("path", rapidjson::Value(request->path().c_str(), allocator), allocator);
    
    if (request->has_requestid()) {
        temp_doc.AddMember("requestId", rapidjson::Value(request->requestid().c_str(), allocator), allocator);
    }
    if (request->has_authorization()) {
        temp_doc.AddMember("authorization", rapidjson::Value(request->authorization().c_str(), allocator), allocator);
    }
    // protobuf 3.12 호환: has_filter()는 proto3 optional 미지원 시 항상 false.
    // filterexp_size() > 0 으로 filter 존재 여부 판단.
    if (request->filter().filterexp_size() > 0) {
        rapidjson::Value filter_val;
        populateFilterExpressions(request->filter(), filter_val, allocator);
        temp_doc.AddMember("filter", filter_val, allocator);
    }
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    temp_doc.Accept(writer);
    std::string final_json_str = buffer.GetString();

    auto request_doc_ptr = JsonSchemaValidator::getInstance().validate(final_json_str);
    if (!request_doc_ptr) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Request validation failed.");
    }

    auto requestData = pool_->acquire();
    requestData->requestId = request->requestid();
    requestData->responseChan = responseChan;
    
    requestData->request_doc->Swap(*request_doc_ptr);
    requestQueue->push(requestData);

    std::shared_ptr<PooledResponseMap> response_doc;
    responseChan->wait_and_pop(response_doc);

    if (response_doc->HasMember("requestId") && (*response_doc)["requestId"].IsString()) {
        response->set_requestid((*response_doc)["requestId"].GetString());
    }
    if (response_doc->HasMember("error")) {
        response->set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        populateErrorResponse((*response_doc)["error"], response->mutable_errorresponse());
    } else if (response_doc->HasMember("data")) {
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        auto success_resp = response->mutable_successresponse();
        auto data_pack = success_resp->mutable_datapack();
        
        const auto& data_val = (*response_doc)["data"];
        if (data_val.IsArray()) {
            for (const auto& data_item : data_val.GetArray()) {
                populateDataPackage(data_item, data_pack->add_data());
            }
        } else {
            populateDataPackage(data_val, data_pack->add_data());
        }
    } else if (response_doc->HasMember("metadata")) {
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        auto success_resp = response->mutable_successresponse();
        const auto& metadata_val = (*response_doc)["metadata"];
        rapidjson::StringBuffer meta_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> meta_writer(meta_buffer);
        metadata_val.Accept(meta_writer);
        success_resp->set_metadata(meta_buffer.GetString());
    }

    if (response_doc->HasMember("ts") && (*response_doc)["ts"].IsString()) {
        response->set_ts((*response_doc)["ts"].GetString());
    }
    
    return grpc::Status::OK;
}

grpc::Status VISSServiceImpl::SetRequest(grpc::ServerContext* context, const grpcProtobufMessages::SetRequestMessage* request, grpcProtobufMessages::SetResponseMessage* response) {
    auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();

    rapidjson::Document temp_doc;
    temp_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = temp_doc.GetAllocator();

    temp_doc.AddMember("action", "set", allocator);
    temp_doc.AddMember("path", rapidjson::Value(request->path().c_str(), allocator), allocator);
    temp_doc.AddMember("value", rapidjson::Value(request->value().c_str(), allocator), allocator);

    if (request->has_requestid()) {
        temp_doc.AddMember("requestId", rapidjson::Value(request->requestid().c_str(), allocator), allocator);
    }
    if (request->has_authorization()) {
        temp_doc.AddMember("authorization", rapidjson::Value(request->authorization().c_str(), allocator), allocator);
    }
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    temp_doc.Accept(writer);
    std::string final_json_str = buffer.GetString();

    auto request_doc_ptr = JsonSchemaValidator::getInstance().validate(final_json_str);
    if (!request_doc_ptr) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Request validation failed.");
    }

    auto requestData = pool_->acquire();
    requestData->requestId = request->requestid();
    requestData->responseChan = responseChan;
    requestData->request_doc->Swap(*request_doc_ptr);
    
    requestQueue->push(requestData);

    std::shared_ptr<PooledResponseMap> response_doc;
    responseChan->wait_and_pop(response_doc);

    if (response_doc->HasMember("requestId") && (*response_doc)["requestId"].IsString()) {
        response->set_requestid((*response_doc)["requestId"].GetString());
    }
    if (response_doc->HasMember("error")) {
        response->set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        populateErrorResponse((*response_doc)["error"], response->mutable_errorresponse());
    } else {
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    }

    if (response_doc->HasMember("ts") && (*response_doc)["ts"].IsString()) {
        response->set_ts((*response_doc)["ts"].GetString());
    }

    return grpc::Status::OK;
}

grpc::Status VISSServiceImpl::SubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::SubscribeRequestMessage* request, grpc::ServerWriter<grpcProtobufMessages::SubscribeStreamMessage>* writer) {
    auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    
    rapidjson::Document temp_doc;
    temp_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = temp_doc.GetAllocator();

    temp_doc.AddMember("action", "subscribe", allocator);
    temp_doc.AddMember("path", rapidjson::Value(request->path().c_str(), allocator), allocator);
    temp_doc.AddMember("requestId", rapidjson::Value(request->requestid().c_str(), allocator), allocator);

    if (request->has_authorization()) {
        temp_doc.AddMember("authorization", rapidjson::Value(request->authorization().c_str(), allocator), allocator);
    }
    // protobuf 3.12 호환: has_filter()는 proto3 optional 미지원 시 항상 false.
    // filterexp_size() > 0 으로 filter 존재 여부 판단.
    if (request->filter().filterexp_size() > 0) {
        rapidjson::Value filter_val;
        populateFilterExpressions(request->filter(), filter_val, allocator);
        temp_doc.AddMember("filter", filter_val, allocator);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer_str(buffer);
    temp_doc.Accept(writer_str);
    std::string final_json_str = buffer.GetString();

    auto request_doc_ptr = JsonSchemaValidator::getInstance().validate(final_json_str);
    if (!request_doc_ptr) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Request validation failed.");
    }

    auto subscribeRequest = pool_->acquire();
    subscribeRequest->requestId = request->requestid();
    subscribeRequest->responseChan = responseChan;
    
    subscribeRequest->request_doc->Swap(*request_doc_ptr);
    requestQueue->push(subscribeRequest);

    std::shared_ptr<PooledResponseMap> initial_response_doc;
    responseChan->wait_and_pop(initial_response_doc);

    grpcProtobufMessages::SubscribeStreamMessage initial_response;
    std::string subscriptionId;
    if (initial_response_doc->HasMember("subscriptionId") && (*initial_response_doc)["subscriptionId"].IsString()) {
        subscriptionId = (*initial_response_doc)["subscriptionId"].GetString();
    }
    
    initial_response.set_mtype(grpcProtobufMessages::SubscribeResponseType::RESPONSE);
    auto resp_msg = initial_response.mutable_response();
    resp_msg->set_requestid(request->requestid());
    if (initial_response_doc->HasMember("ts") && (*initial_response_doc)["ts"].IsString()) {
        resp_msg->set_ts((*initial_response_doc)["ts"].GetString());
    }

    if (initial_response_doc->HasMember("error")) {
        initial_response.set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        if (resp_msg) {
            populateErrorResponse((*initial_response_doc)["error"], resp_msg->mutable_errorresponse());
        }
        writer->Write(initial_response);
        return grpc::Status::OK;
    } else {
        initial_response.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        resp_msg->set_subscriptionid(subscriptionId);
    }

    writer->Write(initial_response);

    bool server_terminated = false;
    while (!context->IsCancelled()) {
        std::shared_ptr<PooledResponseMap> notification_doc;
        LOG_INFO << "Waiting subscription....id:" << subscriptionId;
        responseChan->wait_and_pop(notification_doc);

        if (!notification_doc || (notification_doc->HasMember("status") && (*notification_doc)["status"] == "terminated")) {
            LOG_INFO << "Exit loop on termination signal or null. " << subscriptionId;
            server_terminated = true;
            break;
        }

        grpcProtobufMessages::SubscribeStreamMessage event_msg;
        event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
        event_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        auto event = event_msg.mutable_event();
        
        if (notification_doc->HasMember("subscriptionId") && (*notification_doc)["subscriptionId"].IsString()) {
            event->set_subscriptionid((*notification_doc)["subscriptionId"].GetString());
        }
        if (notification_doc->HasMember("ts") && (*notification_doc)["ts"].IsString()) {
            event->set_ts((*notification_doc)["ts"].GetString());
        }

        if (notification_doc->HasMember("data")) {
            auto success_resp = event->mutable_successresponse();
            auto data_pack = success_resp->mutable_datapack();
            const auto& data_val = (*notification_doc)["data"];

            if (data_val.IsArray()) {
                for (const auto& data_item : data_val.GetArray()) {
                    populateDataPackage(data_item, data_pack->add_data());
                }
            } else {
                populateDataPackage(data_val, data_pack->add_data());
            }
        }
        writer->Write(event_msg);
    }

    // Client-driven teardown: the stream ended by cancellation/disconnect, not
    // by an explicit Unsubscribe. Previously nothing removed the Subscription
    // in that case — the 50ms publisher loop kept process()ing it forever and
    // pushed ~1/sec notifications into this responseChan that no one consumes,
    // growing the queue (and pinning pooled documents) unbounded. Route an
    // internal unsubscribe through the request queue so
    // VssSubscriptionManager::removeSubscription() tears down the subscription
    // and releases its queue. server_terminated means the server side already
    // removed it (explicit unsubscribe path) — nothing left to clean.
    if (!server_terminated && !subscriptionId.empty()) {
        auto teardownChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
        auto teardownRequest = pool_->acquire();
        teardownRequest->requestId = request->requestid();
        teardownRequest->responseChan = teardownChan;
        teardownRequest->request_doc->SetObject();
        rapidjson::Document::AllocatorType& td_alloc = teardownRequest->request_doc->GetAllocator();
        teardownRequest->request_doc->AddMember("action", rapidjson::Value("unsubscribe", td_alloc), td_alloc);
        teardownRequest->request_doc->AddMember("subscriptionId", rapidjson::Value(subscriptionId.c_str(), td_alloc), td_alloc);
        teardownRequest->request_doc->AddMember("requestId", rapidjson::Value(request->requestid().c_str(), td_alloc), td_alloc);
        requestQueue->push(teardownRequest);

        std::shared_ptr<PooledResponseMap> teardown_resp;
        teardownChan->wait_and_pop(teardown_resp);
        LOG_INFO << "gRPC Subscription " << subscriptionId << " auto-unsubscribed on stream teardown.";
    }

    LOG_INFO << "gRPC Subscription " << subscriptionId << " terminated.";
    return grpc::Status::OK;
}

grpc::Status VISSServiceImpl::UnsubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::UnsubscribeRequestMessage* request, grpcProtobufMessages::UnsubscribeResponseMessage* response) {
    auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    
    auto unsubscribeRequest = pool_->acquire();
    // Use the requestId from the incoming request, not the subscriptionId
    unsubscribeRequest->requestId = request->requestid();
    unsubscribeRequest->responseChan = responseChan;

    unsubscribeRequest->request_doc->SetObject();
    rapidjson::Document::AllocatorType& allocator = unsubscribeRequest->request_doc->GetAllocator();

    rapidjson::Value action_val("unsubscribe", allocator);
    rapidjson::Value sub_id_val(request->subscriptionid().c_str(), allocator);
    rapidjson::Value req_id_val(request->requestid().c_str(), allocator);

    unsubscribeRequest->request_doc->AddMember("action", action_val, allocator);
    unsubscribeRequest->request_doc->AddMember("subscriptionId", sub_id_val, allocator);
    // Also add the requestId to the internal JSON document
    unsubscribeRequest->request_doc->AddMember("requestId", req_id_val, allocator);
    
    requestQueue->push(unsubscribeRequest);

    std::shared_ptr<PooledResponseMap> response_doc;
    responseChan->wait_and_pop(response_doc);
    
    // Set the requestId in the final response
    response->set_requestid(request->requestid());
    if (response_doc->HasMember("error")) {
        response->set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        populateErrorResponse((*response_doc)["error"], response->mutable_errorresponse());
    } else {
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    }
    if (response_doc->HasMember("ts") && (*response_doc)["ts"].IsString()) {
        response->set_ts((*response_doc)["ts"].GetString());
    }
    return grpc::Status::OK;
}


// GrpcTransport implementation

GrpcTransport::GrpcTransport(
    std::shared_ptr<ObjectPool<RequestData>> pool,
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue) 
    : m_stopped(false) {
    
    int port = ConfigManager::getInstance().grpc.port;
    server_address = "0.0.0.0:" + std::to_string(port);
    
    service = std::make_unique<VISSServiceImpl>(pool, queue);
}

GrpcTransport::~GrpcTransport() {
    stop();
}

void GrpcTransport::start() {
    grpc::ServerBuilder builder;
    int bound_port = 0;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &bound_port);
    //cq = builder.AddCompletionQueue();
    builder.RegisterService(service.get());
    // Each SubscribeRequest permanently holds a thread for its lifetime.
    // carcare_daemon subscribes to ~50 paths simultaneously, so we need
    // enough threads to handle all concurrent streaming subscriptions.
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::NUM_CQS, 8);
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, 8);
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 256);
    // Allow client keepalive pings at 30s+ intervals (default 5min is too strict
    // and causes ENHANCE_YOUR_CALM/GOAWAY when replay ends and data stops)
    builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 30000);
    server = builder.BuildAndStart();
    if (!server) {
        LOG_ERROR << "Sync gRPC server FAILED to start on " << server_address << " (BuildAndStart returned null)";
        return;
    }
    if (bound_port == 0) {
        LOG_ERROR << "Sync gRPC server FAILED to bind port on " << server_address << " (bound_port=0)";
        server->Shutdown();
        server.reset();
        return;
    }
    LOG_INFO << "Sync gRPC server listening on " << server_address << " (bound_port=" << bound_port << ")";

    server_thread_ = std::thread([this]() {
        this->server->Wait();
    });
    LOG_INFO << "Sync gRPC server thread started.";
}

void GrpcTransport::stop() {
    if (m_stopped.exchange(true)) {
        LOG_WARN << "Sync GrpcTransport::stop() called multiple times.";
        return;
    }

    if (server) {
        LOG_INFO << "Sync GrpcTransport::stop() called. Shutting down gRPC server.";
        server->Shutdown(std::chrono::system_clock::now() + std::chrono::milliseconds(500));

        if (server_thread_.joinable()) {
            LOG_INFO << "Waiting for gRPC server thread to join...";
            server_thread_.join();
            LOG_INFO << "gRPC server thread joined.";
        }

        LOG_INFO << "Sync GrpcTransport::stop() completed.";
    }
}
