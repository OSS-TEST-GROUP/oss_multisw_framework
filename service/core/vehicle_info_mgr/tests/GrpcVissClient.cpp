#include "GrpcVissClient.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <sstream>
#include "Logger.h"

GrpcVissClient::GrpcVissClient() {}

GrpcVissClient::~GrpcVissClient() {
    stop();
}

void GrpcVissClient::start(const std::string& address) {
    m_channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    m_stub = grpcProtobufMessages::VISS::NewStub(m_channel);
    if(m_onEvent) m_onEvent("gRPC Client connected to " + address);
}

void GrpcVissClient::stop() {
    {
        std::lock_guard<std::mutex> lock(m_subMutex);
        for (auto& pair : m_subscriptions) {
            pair.second->stopFlag = true;
            if (pair.second->context) {
                pair.second->context->TryCancel();
            }
        }
        m_subscriptions.clear();
    }
    m_stub.reset();
    m_channel.reset();
    if(m_onEvent) m_onEvent("gRPC Client stopped");
}

void GrpcVissClient::sendRequest(const std::string& jsonReq) {
    rapidjson::Document doc;
    doc.Parse(jsonReq.c_str());
    if (doc.HasParseError()) {
        if(m_onResponse) m_onResponse("JSON Parse Error");
        return;
    }

    std::string action = doc.HasMember("action") ? doc["action"].GetString() : "";
    std::string path = doc.HasMember("path") ? doc["path"].GetString() : "";
    std::string value = doc.HasMember("value") ? doc["value"].GetString() : "";
    std::string subId = doc.HasMember("subscriptionId") ? doc["subscriptionId"].GetString() : "";
    std::string reqId = doc.HasMember("requestId") ? doc["requestId"].GetString() : "";

    if (action == "get") {
        std::string filterStr = "";
        if (doc.HasMember("filter")) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc["filter"].Accept(writer);
            filterStr = buffer.GetString();
        }
        Get(path, filterStr, reqId);
    } else if (action == "set") {
        Set(path, value, reqId);
    } else if (action == "subscribe") {
        std::string filterStr = "";
        if (doc.HasMember("filter")) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc["filter"].Accept(writer);
            filterStr = buffer.GetString();
        }
        // 기존 구독을 중단하지 않고 새 스레드 생성
        std::thread(&GrpcVissClient::Subscribe, this, path, filterStr, reqId).detach();
    } else if (action == "unsubscribe") {
        if (!subId.empty()) {
            Unsubscribe(subId, reqId);
        }
    } else {
        if(m_onResponse) m_onResponse("Unknown action: " + action);
    }
}

void GrpcVissClient::Get(const std::string& path, const std::string& jsonFilter, const std::string& reqId) {
    if (!m_stub) return;
    grpcProtobufMessages::GetRequestMessage request;
    request.set_path(path);
    request.set_requestid(reqId.empty() ? ("req-get-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())) : reqId);

    if (!jsonFilter.empty()) {
        parseFilter(jsonFilter, request.mutable_filter());
    }

    grpcProtobufMessages::GetResponseMessage response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

    grpc::Status status = m_stub->GetRequest(&context, request, &response);
    if (status.ok()) {
        if (response.status() == grpcProtobufMessages::ResponseStatus::SUCCESS) {
            if (response.has_successresponse() && response.successresponse().has_datapack() && 
                response.successresponse().datapack().data_size() > 0 && 
                response.successresponse().datapack().data(0).dp_size() > 0) {
                if(m_onResponse) m_onResponse("Get Result: " + response.successresponse().datapack().data(0).dp(0).value());
            } else {
                if(m_onResponse) m_onResponse("Get Success (No Data)");
            }
        } else {
            if(m_onResponse) m_onResponse("Get Error: " + response.errorresponse().reason());
        }
    } else {
        if(m_onResponse) m_onResponse("gRPC Error: " + status.error_message());
    }
}

void GrpcVissClient::Set(const std::string& path, const std::string& value, const std::string& reqId) {
    if (!m_stub) return;
    grpcProtobufMessages::SetRequestMessage request;
    request.set_path(path);
    request.set_value(value);
    request.set_requestid(reqId.empty() ? ("req-set-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())) : reqId);

    grpcProtobufMessages::SetResponseMessage response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

    grpc::Status status = m_stub->SetRequest(&context, request, &response);
    if (status.ok()) {
        if (response.status() == grpcProtobufMessages::ResponseStatus::SUCCESS) {
            if(m_onResponse) m_onResponse("Set Success");
        } else {
            if(m_onResponse) m_onResponse("Set Error: " + response.errorresponse().reason());
        }
    } else {
        if(m_onResponse) m_onResponse("gRPC Error: " + status.error_message());
    }
}

void GrpcVissClient::Subscribe(const std::string& path, const std::string& jsonFilter, const std::string& reqId) {
    if (!m_stub) return;

    auto info = std::make_shared<SubscriptionInfo>();
    info->context = std::make_shared<grpc::ClientContext>();

    grpcProtobufMessages::SubscribeRequestMessage request;
    request.set_path(path);
    request.set_requestid(reqId.empty() ? ("req-sub-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())) : reqId);

    if (!jsonFilter.empty()) {
        parseFilter(jsonFilter, request.mutable_filter());
    }

    auto reader = m_stub->SubscribeRequest(info->context.get(), request);

    grpcProtobufMessages::SubscribeStreamMessage response;
    if (!reader->Read(&response)) {
        if(m_onResponse) m_onResponse("Subscribe: Initial Read failed (Server disconnected?)");
        reader->Finish(); // 리소스 정리
        return;
    }

    if (response.status() != grpcProtobufMessages::ResponseStatus::SUCCESS) {
        std::string reason = "Unknown error";
        if (response.has_response() && response.response().has_errorresponse()) {
            reason = response.response().errorresponse().reason();
        }
        if(m_onResponse) m_onResponse("Subscribe Error: " + reason);
        reader->Finish(); // 리소스 정리
        return;
    }

    std::string subId = response.response().subscriptionid();
    if(m_onResponse) m_onResponse("Subscribed [" + path + "]. ID: " + subId);
    if(m_onSubscription) m_onSubscription(path, subId);

    {
        std::lock_guard<std::mutex> lock(m_subMutex);
        m_subscriptions[subId] = info;
    }

    while (!info->stopFlag.load()) {
        if (reader->Read(&response)) {
                auto& datapack = response.event().successresponse().datapack();
                
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                writer.StartObject();
                writer.Key("action"); writer.String("subscription");
                writer.Key("subscriptionId"); writer.String(subId.c_str());
                
                // Determine if it should be an array or object for "data"
                // To support all cases, we follow the user's "multiple nodes" case when size > 1
                if (datapack.data_size() > 1) {
                    writer.Key("data");
                    writer.StartArray();
                    for (int i = 0; i < datapack.data_size(); ++i) {
                        const auto& d = datapack.data(i);
                        writer.StartObject();
                        writer.Key("path"); writer.String(d.path().c_str());
                        
                        if (d.dp_size() > 1) {
                            writer.Key("dp");
                            writer.StartArray();
                            for (int j = 0; j < d.dp_size(); ++j) {
                                writer.StartObject();
                                writer.Key("ts"); writer.String(d.dp(j).ts().c_str());
                                writer.Key("value"); writer.String(d.dp(j).value().c_str());
                                writer.EndObject();
                            }
                            writer.EndArray();
                        } else {
                            writer.Key("dp");
                            writer.StartObject();
                            writer.Key("ts"); writer.String(d.dp(0).ts().c_str());
                            writer.Key("value"); writer.String(d.dp(0).value().c_str());
                            writer.EndObject();
                        }
                        writer.EndObject();
                    }
                    writer.EndArray();
                } else {
                    // Single data node case
                    const auto& d = datapack.data(0);
                    writer.Key("data");
                    writer.StartObject();
                    writer.Key("path"); writer.String(d.path().c_str());
                    
                    if (d.dp_size() > 1) {
                        writer.Key("dp");
                        writer.StartArray();
                        for (int j = 0; j < d.dp_size(); ++j) {
                            writer.StartObject();
                            writer.Key("ts"); writer.String(d.dp(j).ts().c_str());
                            writer.Key("value"); writer.String(d.dp(j).value().c_str());
                            writer.EndObject();
                        }
                        writer.EndArray();
                    } else {
                        writer.Key("dp");
                        writer.StartObject();
                        writer.Key("ts"); writer.String(d.dp(0).ts().c_str());
                        writer.Key("value"); writer.String(d.dp(0).value().c_str());
                        writer.EndObject();
                    }
                    writer.EndObject();
                }
                
                writer.Key("ts"); writer.String(response.event().ts().c_str());
                writer.EndObject();

                if(m_onEvent) m_onEvent(buffer.GetString());
        } else {
            LOG_INFO << "Subscribe: Stream closed or Read failed for " << subId;
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_subMutex);
        m_subscriptions.erase(subId);
    }
    reader->Finish();
}

void GrpcVissClient::Unsubscribe(const std::string& subId, const std::string& reqId) {
    if (!m_stub) return;
    grpcProtobufMessages::UnsubscribeRequestMessage request;
    request.set_subscriptionid(subId);
    request.set_requestid(reqId.empty() ? ("req-unsub-" + subId) : reqId);

    grpcProtobufMessages::UnsubscribeResponseMessage response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

    grpc::Status status = m_stub->UnsubscribeRequest(&context, request, &response);
    if (status.ok()) {
        if (response.status() == grpcProtobufMessages::ResponseStatus::SUCCESS) {
            if(m_onResponse) m_onResponse("Unsubscribe Success: " + subId);
            
            std::lock_guard<std::mutex> lock(m_subMutex);
            if (m_subscriptions.count(subId)) {
                m_subscriptions[subId]->stopFlag = true;
                m_subscriptions[subId]->context->TryCancel();
            }
        } else {
            if(m_onResponse) m_onResponse("Unsubscribe Error: " + response.errorresponse().reason());
        }
    } else {
        if(m_onResponse) m_onResponse("gRPC Error: " + status.error_message());
    }
}
void GrpcVissClient::parseFilter(const std::string& jsonFilter, grpcProtobufMessages::FilterExpressions* filterExprs) {
    if (jsonFilter.empty() || !filterExprs) return;

    rapidjson::Document doc;
    doc.Parse(jsonFilter.c_str());
    if (doc.HasParseError()) return;

    auto addFilter = [&](const rapidjson::Value& f) {
        if (!f.IsObject() || !f.HasMember("variant") || !f.HasMember("parameter")) return;
        std::string variant = f["variant"].GetString();
        const rapidjson::Value& p = f["parameter"];
        
        auto exp = filterExprs->add_filterexp();
        auto v = exp->mutable_value();

        if (variant == "timebased") {
            exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_TIMEBASED);
            if (p.IsObject() && p.HasMember("period")) {
                v->mutable_valuetimebased()->set_period(p["period"].IsString() ? p["period"].GetString() : std::to_string(p["period"].GetInt()));
            } else if (p.IsString()) {
                v->mutable_valuetimebased()->set_period(p.GetString());
            }
        } else if (variant == "range") {
            exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_RANGE);
            auto addRangeItem = [&](const rapidjson::Value& obj) {
                if (!obj.IsObject()) return;
                auto r = v->add_valuerange();
                if (obj.HasMember("logic-op")) {
                    r->set_logicoperator(obj["logic-op"].GetString());
                }
                if (obj.HasMember("boundary")) {
                    r->set_boundary(obj["boundary"].IsString() ? obj["boundary"].GetString() : std::to_string(obj["boundary"].GetInt()));
                }
                /* 아직 proto 파일에서 지원되지 않음.
                if(obj.HasMember("combination-op")){
                    r->set_combinationoperator(obj["combination-op"].GetString());
                }
                */
            };

            if (p.IsArray()) {
                for (auto& item : p.GetArray()) addRangeItem(item);
            } else if (p.IsObject()) {
                addRangeItem(p);
            }
        } else if (variant == "change") {
            exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_CHANGE);
            auto change = v->mutable_valuechange();
            if (p.IsObject()) {
                if (p.HasMember("diff")) change->set_diff(p["diff"].IsString() ? p["diff"].GetString() : std::to_string(p["diff"].GetInt()));
                if (p.HasMember("logic-op")) {
                    change->set_logicoperator(p["logic-op"].GetString());
                }
            }
        } else if (variant == "history") {
            exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_HISTORY);
            if (p.IsString()) v->mutable_valuehistory()->set_timeperiod(p.GetString());
        } else if (variant == "paths") {
            exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_PATHS);
            auto pathsVal = v->mutable_valuepaths();
            if (p.IsArray()) {
                for (auto& item : p.GetArray()) {
                    if (item.IsString()) pathsVal->add_relativepath(item.GetString());
                }
            }
        } else if (variant == "curvelog") {
            exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_CURVELOG);
            auto curve = v->mutable_valuecurvelog();
            if (p.IsObject()) {
                if (p.HasMember("maxErr")) curve->set_maxerr(p["maxErr"].IsString() ? p["maxErr"].GetString() : std::to_string(p["maxErr"].GetDouble()));
                if (p.HasMember("bufSize")) curve->set_bufsize(p["bufSize"].IsString() ? p["bufSize"].GetString() : std::to_string(p["bufSize"].GetInt()));
            }
        } else if (variant == "metadata") {
            exp->set_variant(grpcProtobufMessages::FilterExpressions_FilterExpression_FilterVariant_METADATA);
            if (p.IsString()) v->mutable_valuemetadata()->set_tree(p.GetString());
        }
    };

    if (doc.IsArray()) {
        for (auto& f : doc.GetArray()) addFilter(f);
    } else {
        addFilter(doc);
    }
}
