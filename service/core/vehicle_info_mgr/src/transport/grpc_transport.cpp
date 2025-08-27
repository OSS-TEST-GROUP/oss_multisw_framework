#include <iostream>
#include <google/protobuf/util/json_util.h>
#include "grpc_transport.h"
#include "VissServerApp.h" // For VssSubscriptionManager
#include "Utils.h"
#include "JsonSchemaValidator.h"

// Helper function to populate a DataPackage from a JSON object
void populateDataPackage(const rapidjson::Value& data_item, grpcProtobufMessages::DataPackages::DataPackage* data_package) {
    if (!data_item.IsObject()) return;

    if (data_item.HasMember("path") && data_item["path"].IsString()) {
        data_package->set_path(data_item["path"].GetString());
    }

    if (data_item.HasMember("dp") && data_item["dp"].IsObject()) {
        const auto& dp_obj = data_item["dp"];
        auto dp = data_package->add_dp();
        if (dp_obj.HasMember("value")) {
            rapidjson::StringBuffer val_buf;
            rapidjson::Writer<rapidjson::StringBuffer> val_writer(val_buf);
            dp_obj["value"].Accept(val_writer);
            dp->set_value(val_buf.GetString());
        }
        if (dp_obj.HasMember("ts") && dp_obj["ts"].IsString()) {
            dp->set_ts(dp_obj["ts"].GetString());
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
    
    // Manually construct JSON from proto message to ensure camelCase keys, following reference implementation.
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
    // Note: Filter and DataCompression not implemented in this manual conversion for simplicity.
    // A full implementation would require converting the nested filter message.
    
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

    // Populate gRPC response from the JSON document
    if (response_doc->HasMember("requestId") && (*response_doc)["requestId"].IsString()) {
        response->set_requestid((*response_doc)["requestId"].GetString());
    }
    if (response_doc->HasMember("error") && (*response_doc)["error"].IsObject()) {
        response->set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        const auto& error_obj = (*response_doc)["error"];
        auto error_resp = response->mutable_errorresponse();
        if (error_obj.HasMember("number") && error_obj["number"].IsString()) {
             error_resp->set_number(error_obj["number"].GetString());
        }
        if (error_obj.HasMember("reason") && error_obj["reason"].IsString()) {
             error_resp->set_reason(error_obj["reason"].GetString());
        }
        if (error_obj.HasMember("description") && error_obj["description"].IsString()) {
             error_resp->set_description(error_obj["description"].GetString());
        }
    } else if (response_doc->HasMember("data")) {
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        auto success_resp = response->mutable_successresponse();
        auto data_pack = success_resp->mutable_datapack();
        
        const auto& data_val = (*response_doc)["data"];
        if (data_val.IsArray()) {
            // Handle array for wildcard paths
            for (const auto& data_item : data_val.GetArray()) {
                populateDataPackage(data_item, data_pack->add_data());
            }
        } else if (data_val.IsObject()) {
            // Handle single object for a single path
            populateDataPackage(data_val, data_pack->add_data());
        }
    }
    
    return grpc::Status::OK;
}

grpc::Status VISSServiceImpl::SetRequest(grpc::ServerContext* context, const grpcProtobufMessages::SetRequestMessage* request, grpcProtobufMessages::SetResponseMessage* response) {
    auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();

    // Manually construct JSON from proto message to ensure camelCase keys.
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

    // Populate gRPC response from the JSON document
    if (response_doc->HasMember("requestId") && (*response_doc)["requestId"].IsString()) {
        response->set_requestid((*response_doc)["requestId"].GetString());
    }
    if (response_doc->HasMember("error") && (*response_doc)["error"].IsObject()) {
        response->set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        const auto& error_obj = (*response_doc)["error"];
        auto error_resp = response->mutable_errorresponse();
        if (error_obj.HasMember("number") && error_obj["number"].IsString()) {
             error_resp->set_number(error_obj["number"].GetString());
        }
        if (error_obj.HasMember("reason") && error_obj["reason"].IsString()) {
             error_resp->set_reason(error_obj["reason"].GetString());
        }
        if (error_obj.HasMember("description") && error_obj["description"].IsString()) {
             error_resp->set_description(error_obj["description"].GetString());
        }
    } else {
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    }

    return grpc::Status::OK;
}

grpc::Status VISSServiceImpl::SubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::SubscribeRequestMessage* request, grpc::ServerWriter<grpcProtobufMessages::SubscribeStreamMessage>* writer) {
    auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    
    // Manually construct JSON from proto message to ensure camelCase keys.
    rapidjson::Document temp_doc;
    temp_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = temp_doc.GetAllocator();

    temp_doc.AddMember("action", "subscribe", allocator);
    temp_doc.AddMember("path", rapidjson::Value(request->path().c_str(), allocator), allocator);
    temp_doc.AddMember("requestId", rapidjson::Value(request->requestid().c_str(), allocator), allocator);

    if (request->has_authorization()) {
        temp_doc.AddMember("authorization", rapidjson::Value(request->authorization().c_str(), allocator), allocator);
    }
    // Note: Filter and DataCompression not implemented in this manual conversion for simplicity.

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

    // Wait for the initial response which contains the subscriptionId
    std::shared_ptr<PooledResponseMap> initial_response_doc;
    responseChan->wait_and_pop(initial_response_doc);

    grpcProtobufMessages::SubscribeStreamMessage initial_response;
    std::string subscriptionId;
    if (initial_response_doc->HasMember("subscriptionId") && (*initial_response_doc)["subscriptionId"].IsString()) {
        subscriptionId = (*initial_response_doc)["subscriptionId"].GetString();
    }
    
    // Form and send the initial subscription response
    initial_response.set_mtype(grpcProtobufMessages::SubscribeResponseType::RESPONSE);
    auto resp_msg = initial_response.mutable_response();
    if (initial_response_doc->HasMember("error") && (*initial_response_doc)["error"].IsObject()) {
        initial_response.set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        const auto& error_obj = (*initial_response_doc)["error"];
        auto error_resp = resp_msg->mutable_errorresponse();
        if (error_obj.HasMember("number") && error_obj["number"].IsString()) {
             error_resp->set_number(error_obj["number"].GetString());
        }
        if (error_obj.HasMember("reason") && error_obj["reason"].IsString()) {
             error_resp->set_reason(error_obj["reason"].GetString());
        }
        if (error_obj.HasMember("description") && error_obj["description"].IsString()) {
             error_resp->set_description(error_obj["description"].GetString());
        }
    } else {
        initial_response.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        resp_msg->set_subscriptionid(subscriptionId);
    }
    resp_msg->set_requestid(request->requestid());
    // TODO: Set timestamp
    writer->Write(initial_response);

    // Now, loop and wait for notifications on the same channel
    while (!context->IsCancelled()) {
        std::shared_ptr<PooledResponseMap> notification_doc;
        responseChan->wait_and_pop(notification_doc); 

        if (!notification_doc || (notification_doc->HasMember("status") && (*notification_doc)["status"] == "terminated")) {
            break; // Exit loop on termination signal or null
        }

        grpcProtobufMessages::SubscribeStreamMessage event_msg;
        event_msg.set_mtype(grpcProtobufMessages::SubscribeResponseType::EVENT);
        event_msg.set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
        auto event = event_msg.mutable_event();
        
        if (notification_doc->HasMember("subscriptionId") && (*notification_doc)["subscriptionId"].IsString()) {
            event->set_subscriptionid((*notification_doc)["subscriptionId"].GetString());
        }

        if (notification_doc->HasMember("data") && (*notification_doc)["data"].IsArray()) {
        auto success_resp = event->mutable_successresponse();
        auto data_pack = success_resp->mutable_datapack();
        if (notification_doc->HasMember("data") && (*notification_doc)["data"].IsArray()) {
            for (const auto& data_obj : (*notification_doc)["data"].GetArray()) {
                auto new_dp = data_pack->add_data();
                if (data_obj.HasMember("path") && data_obj["path"].IsString()) {
                    new_dp->set_path(data_obj["path"].GetString());
                }
                if (data_obj.HasMember("dp") && data_obj["dp"].IsObject()) {
                    const auto& dp_obj = data_obj["dp"];
                    auto point = new_dp->add_dp();
                    if (dp_obj.HasMember("value")) {
                        rapidjson::StringBuffer val_buf;
                        rapidjson::Writer<rapidjson::StringBuffer> val_writer(val_buf);
                        dp_obj["value"].Accept(val_writer);
                        point->set_value(val_buf.GetString());
                    }
                    if (dp_obj.HasMember("ts") && dp_obj["ts"].IsString()) {
                        point->set_ts(dp_obj["ts"].GetString());
                    }
                }
            }
        }
        }
        writer->Write(event_msg);
    }
    
    // No need to call removeSubscription here, as the client disconnecting (IsCancelled)
    // or an unsubscribe message will handle it.
    std::cout << "gRPC Subscription " << subscriptionId << " terminated." << std::endl;
    return grpc::Status::OK;
}

grpc::Status VISSServiceImpl::UnsubscribeRequest(grpc::ServerContext* context, const grpcProtobufMessages::UnsubscribeRequestMessage* request, grpcProtobufMessages::UnsubscribeResponseMessage* response) {
    auto responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    
    auto unsubscribeRequest = pool_->acquire();
    // For unsubscribe, the subscriptionId from the proto is used as the core requestId
    unsubscribeRequest->requestId = request->subscriptionid();
    unsubscribeRequest->responseChan = responseChan;

    // Create a JSON document for the unsubscribe action
    unsubscribeRequest->request_doc->SetObject();
    rapidjson::Value action_val("unsubscribe", unsubscribeRequest->request_doc->GetAllocator());
    rapidjson::Value sub_id_val(request->subscriptionid().c_str(), unsubscribeRequest->request_doc->GetAllocator());

    unsubscribeRequest->request_doc->AddMember("action", action_val, unsubscribeRequest->request_doc->GetAllocator());
    unsubscribeRequest->request_doc->AddMember("subscriptionId", sub_id_val, unsubscribeRequest->request_doc->GetAllocator());
    
    // Unsubscribe doesn't need schema validation as it's a simple, internally generated message
    
    requestQueue->push(unsubscribeRequest);

    std::shared_ptr<PooledResponseMap> response_doc;
    responseChan->wait_and_pop(response_doc);
    
    response->set_requestid(request->requestid());
    if (response_doc->HasMember("error") && (*response_doc)["error"].IsObject()) {
        response->set_status(grpcProtobufMessages::ResponseStatus::ERROR);
        const auto& error_obj = (*response_doc)["error"];
        auto error_resp = response->mutable_errorresponse();
        if (error_obj.HasMember("number") && error_obj["number"].IsString()) {
             error_resp->set_number(error_obj["number"].GetString());
        }
        if (error_obj.HasMember("reason") && error_obj["reason"].IsString()) {
             error_resp->set_reason(error_obj["reason"].GetString());
        }
        if (error_obj.HasMember("description") && error_obj["description"].IsString()) {
             error_resp->set_description(error_obj["description"].GetString());
        }
    } else {
        response->set_status(grpcProtobufMessages::ResponseStatus::SUCCESS);
    }
    return grpc::Status::OK;
}


// GrpcTransport implementation
GrpcTransport::GrpcTransport(
    std::shared_ptr<ObjectPool<RequestData>> pool,
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue) 
    : server_address("0.0.0.0:50051") {
    service = std::make_unique<VISSServiceImpl>(pool, queue);
}

GrpcTransport::~GrpcTransport() {
    stop();
}

void GrpcTransport::start() {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    server = builder.BuildAndStart();
    std::cout << "gRPC server listening on " << server_address << std::endl;
    server->Wait();
}

void GrpcTransport::stop() {
    if (server) {
        server->Shutdown();
    }
}
