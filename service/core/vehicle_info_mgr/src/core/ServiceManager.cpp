#include "ServiceManager.h"
#include "VssTree.h"
#include "Utils.h"
#include "StatusCodes.h"
#include "VssSubscriptionManager.h"
#include <iostream>
#include <algorithm>
#include <typeinfo>
#include "Logger.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

// ################### ServiceManager Implementation ###################
ServiceManager::ServiceManager(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> req_q, std::shared_ptr<viss::VSSTree> vss_tree)
    : requestQueue(req_q), vss_tree_(vss_tree), active(false) {
    response_pool_ = std::make_shared<ObjectPool<PooledResponseMap>>();
    auto data_store_ptr = std::make_shared<VssDataStore>();
    storage = data_store_ptr;
    dataChannel = std::make_shared<ThreadSafeQueue<VehicleData>>();
    subscription_manager = std::make_shared<VssSubscriptionManager>(storage, response_pool_, vss_tree_);
}

ServiceManager::~ServiceManager() {
    stop();
}

std::shared_ptr<VssSubscriptionManager> ServiceManager::getSubscriptionManager() {
    return subscription_manager;
}

void ServiceManager::addProvider(std::unique_ptr<IProvider> provider) {
    provider->setDataChannel(dataChannel);
    provider->setVssTree(vss_tree_); // Inject the VSS tree
    providers.push_back(std::move(provider));
}

void ServiceManager::run() {
    active = true;
    subscription_manager->run();
    data_channel_thread = std::thread(&ServiceManager::dataChannelLoop, this);

    for (auto& provider : providers) {
        provider->start();
    }

    while(active.load()) {
        std::shared_ptr<RequestData> request;
        requestQueue->wait_and_pop(request);
        if (!active.load() || !request) {
            LOG_INFO << "Request processing loop is shutting down.";
            break;
        }
        
        std::cout << "--- Processing Request ---" << std::endl;
        request->print();
        
        processRequest(std::move(request));
    }
}

void ServiceManager::stop() {
    if (active.load()) {
        LOG_INFO << "ServiceManager::stop() called.";
        active = false;
        subscription_manager->stop();
        for (auto& provider : providers) {
            provider->stop();
        }
        VehicleData dummy_data;
        dummy_data.path = "shutdown";
        dataChannel->push(dummy_data);
        if (data_channel_thread.joinable()) {
            data_channel_thread.join();
        }
        requestQueue->push(nullptr); // Push a null pointer to signal shutdown
        LOG_INFO << "ServiceManager::stop() completed.";
    }
}

void ServiceManager::dataChannelLoop() {
    while (active.load()) {
        VehicleData data;
        dataChannel->wait_and_pop(data);
        if (!active.load() || data.path == "shutdown") {
            LOG_INFO << "Data channel loop is shutting down.";
            break;
        }
        // This will now trigger the onDataChanged callback in VssSubscriptionManager
        storage->setValue(data);
    }
}

void ServiceManager::processRequest(std::shared_ptr<RequestData> request) {
    const auto& req_doc = *request->request_doc;
    std::string action = req_doc["action"].GetString();

    auto response_doc = response_pool_->acquire();
    rapidjson::Document::AllocatorType& allocator = response_doc->GetAllocator();
    response_doc->SetObject();
    
    response_doc->AddMember("action", rapidjson::Value(action.c_str(), allocator), allocator);
    response_doc->AddMember("requestId", rapidjson::Value(request->requestId.c_str(), allocator), allocator);

    if (action == "unsubscribe") {
        if (req_doc.HasMember("subscriptionId") && req_doc["subscriptionId"].IsString()) {
            std::string subscriptionId = req_doc["subscriptionId"].GetString();
            if (subscription_manager->removeSubscription(subscriptionId)) {
                response_doc->AddMember("subscriptionId", rapidjson::Value(subscriptionId.c_str(), allocator), allocator);
            } else {
                response_doc->AddMember("error", rapidjson::Value(StatusCodes::NOT_FOUND("Subscription ID not found."), allocator), allocator);
            }
        } else {
            response_doc->AddMember("error", rapidjson::Value(StatusCodes::BAD_REQUEST("Unsubscribe request must contain a 'subscriptionId'."), allocator), allocator);
        }
    } else if (action == "get") {
        std::string path = req_doc["path"].GetString();
        std::vector<std::string> paths = vss_tree_->getPaths(path, request->request_doc.get());

        if (paths.empty()) {
            response_doc->AddMember("error", rapidjson::Value(StatusCodes::NOT_FOUND(), allocator), allocator);
        } else {
            // Find if a history filter exists to get the time period
            std::string timePeriod = "";
            bool is_get_metadata = false;
            int depth = 0;

            if (req_doc.HasMember("filter")) {
                const auto& filter_val = req_doc["filter"];
                for (const auto& filter_item : filter_val.GetArray()) {
                    if (filter_item.IsObject() && filter_item.HasMember("variant") && filter_item["variant"].IsString()) {
                        auto variant = filter_item["variant"].GetString();
                        if (strcmp(variant, "metadata") == 0) {
                            is_get_metadata = true;
                            if (filter_item.HasMember("parameter") && filter_item["parameter"].IsArray() && !filter_item["parameter"].Empty() && filter_item["parameter"][0].IsString()) {
                                try {
                                    depth = std::stoi(filter_item["parameter"][0].GetString());
                                } catch (const std::invalid_argument& ia) {
                                    LOG_ERROR << "Invalid argument for depth: " << filter_item["parameter"][0].GetString();
                                    depth = 0; // Default to 0 on error
                                }
                            }
                        } else if (strcmp(variant, "history") == 0) {
                            if (filter_item.HasMember("parameter") && filter_item["parameter"].IsArray() && !filter_item["parameter"].Empty() && filter_item["parameter"][0].IsString()) {
                                timePeriod = filter_item["parameter"][0].GetString();
                            }
                        }
                    }
                }
            }
            
            if (is_get_metadata) {
                 rapidjson::Value metadata_array(rapidjson::kArrayType);
                for (const auto& p : paths) {
                    try {
                        rapidjson::Document metadata_doc = vss_tree_->getMetadata(p, depth);
                        if (!metadata_doc.ObjectEmpty()) {
                            rapidjson::Value metadata_copy(metadata_doc, allocator);
                            metadata_array.PushBack(metadata_copy, allocator);
                        }
                    } catch (const std::exception& e) {
                        LOG_WARN << "Could not get metadata for path " << p << ": " << e.what();
                    }
                }

                if (metadata_array.Size() == 1) {
                    response_doc->AddMember("metadata", metadata_array[0], allocator);
                } else {
                    response_doc->AddMember("metadata", metadata_array, allocator);
                }
            } else {
                rapidjson::Value data_array(rapidjson::kArrayType);
                for (const auto& p : paths) {
                auto values = storage->getValues(p, timePeriod);
                if (values.empty()) {
                    LOG_WARN << "Values are empty for path " << p;
                    continue;
                }

                rapidjson::Value dp_list(rapidjson::kArrayType);
                for (const auto& point : values) {
                    rapidjson::Value dp_obj(rapidjson::kObjectType);
                    dp_obj.AddMember("value", any_to_json_value(point.value, allocator), allocator);
                    dp_obj.AddMember("ts", rapidjson::Value(formatTimestamp(point.timestamp).c_str(), allocator), allocator);
                    dp_list.PushBack(dp_obj, allocator);
                }

                rapidjson::Value data_obj(rapidjson::kObjectType);
                data_obj.AddMember("path", rapidjson::Value(p.c_str(), allocator), allocator);

                if (dp_list.Size() == 1) {
                    data_obj.AddMember("dp", dp_list[0], allocator);
                } else {
                    data_obj.AddMember("dp", dp_list, allocator);
                }
                data_array.PushBack(data_obj, allocator);
            }

            if (data_array.Size() == 1) {
                response_doc->AddMember("data", data_array[0], allocator);
            } else {
                response_doc->AddMember("data", data_array, allocator);
            }
        }
    }
    } else if (action == "subscribe") {
        std::string path = req_doc["path"].GetString();
        if (vss_tree_->getPaths(path, request->request_doc.get()).empty()) {
            response_doc->AddMember("error", rapidjson::Value(StatusCodes::NOT_FOUND("The path is not available in the VSS tree. " + path), allocator), allocator);
        } else {
            std::string subscriptionId = subscription_manager->addSubscription(request);
            if(subscriptionId.empty()) {
                response_doc->AddMember("error", rapidjson::Value(StatusCodes::BAD_REQUEST("The request is malformed or invalid. "), allocator), allocator);
            } else {
                response_doc->AddMember("subscriptionId", rapidjson::Value(subscriptionId.c_str(), allocator), allocator);
            }
        }
    } else if (action == "set") {
        std::string path = req_doc["path"].GetString();
        auto node = vss_tree_->findNode(path);
        if (node == nullptr) {
            response_doc->AddMember("error", rapidjson::Value(StatusCodes::NOT_FOUND(), allocator), allocator);
        } else if (node->NodeType != viss::NodeTypes::ACTUATOR) {
            response_doc->AddMember("error", rapidjson::Value(StatusCodes::INVALID_DATA("Update of a " + node->getTypeAsString() + " is not supported."), allocator), allocator);
        } else {
            /*
            const auto& value_node = req_doc["value"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            value_node.Accept(writer);
            std::string value_str = buffer.GetString();
            */
            std::string value_str = req_doc["value"].GetString();

            VehicleData data;
            data.path = path;
            data.value = value_str;
            data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            
            storage->setValue(data);
            for (const auto& provider : providers) {
                provider->setValue(path, value_str);
            }
        }
    } else {
        response_doc->AddMember("error", rapidjson::Value(StatusCodes::BAD_REQUEST(), allocator), allocator);
    }

    response_doc->AddMember("ts", rapidjson::Value(getCurrentTimestamp().c_str(), allocator), allocator);
    
    if (request->responseChan) {
        std::cout << "Sending response for requestId: " << request->requestId << " to transport layer." << std::endl;
        request->responseChan->push(response_doc);
    }
}
