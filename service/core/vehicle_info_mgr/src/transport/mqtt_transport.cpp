#include "mqtt/async_client.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <any>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "VissServerApp.h"
#include "mqtt_transport.h"
#include "Utils.h"
#include "JsonSchemaValidator.h"

// --- MQTT Configuration ---
const std::string MQTT_SERVER_ADDRESS("tcp://localhost:1883");
const std::string MQTT_CLIENT_ID("viss_server_mqtt_transport");
const std::string VISS_REQUEST_TOPIC("VISS/request/#");
const std::string VISS_RESPONSE_TOPIC_PREFIX("VISS/response/");

// Forward declaration
class MqttCallbackHandler;

// MqttTransport class implementation details
struct MqttTransportImpl {
    mqtt::async_client client;
    mqtt::connect_options connOpts;
    std::unique_ptr<MqttCallbackHandler> callback;
    std::shared_ptr<ObjectPool<RequestData>> pool;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>> responseChan;
    std::mutex topic_map_mutex_;
    std::map<std::string, std::string> request_topic_map_;
    std::mutex subscription_map_mutex_;
    std::map<std::string, std::string> subscription_topic_map_;

    std::thread response_thread_;
    std::atomic<bool> stop_flag_{false};

    MqttTransportImpl(const std::string& server_uri, const std::string& client_id, std::shared_ptr<ObjectPool<RequestData>> p, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue)
        : client(server_uri, client_id)
        , pool(p)
        , requestQueue(queue)
        , responseChan(std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>())
    {
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);

        response_thread_ = std::thread([this]() {
            while (!stop_flag_.load()) {
                std::shared_ptr<PooledResponseMap> response_doc;
                responseChan->wait_and_pop(response_doc);

                if (stop_flag_.load() || !response_doc) break;

                std::string action;
                std::string requestId;
                std::string topic;

                if (response_doc->HasMember("action") && (*response_doc)["action"].IsString()) {
                    action = (*response_doc)["action"].GetString();
                } else {
                    std::cerr << "Error processing response: 'action' is missing or not a string." << std::endl;
                    continue;
                }

                if (response_doc->HasMember("requestId") && (*response_doc)["requestId"].IsString()) {
                    requestId = (*response_doc)["requestId"].GetString();
                    std::lock_guard<std::mutex> lock(topic_map_mutex_);
                    auto it = request_topic_map_.find(requestId);
                    if (it != request_topic_map_.end()) {
                        topic = it->second;
                        request_topic_map_.erase(it);
                    }
                }

                if (response_doc->HasMember("subscriptionId") && (*response_doc)["subscriptionId"].IsString()) {
                    std::string subscriptionId = (*response_doc)["subscriptionId"].GetString();
                    if (!response_doc->HasMember("error")) {
                        std::lock_guard<std::mutex> lock(subscription_map_mutex_);
                        if (action == "subscribe") {
                            subscription_topic_map_[subscriptionId] = topic;
                        } else if (action == "unsubscribe") {
                            subscription_topic_map_.erase(subscriptionId);
                            response_doc->EraseMember("subscriptionId");
                        } else if (action == "subscription") {
                            auto it = subscription_topic_map_.find(subscriptionId);
                            if (it != subscription_topic_map_.end()) {
                                topic = it->second;
                            }
                        }
                    }
                }

                if (topic.empty()) {
                    std::cerr << "Error: Response topic could not be determined for requestId: " << requestId << std::endl;
                    continue;
                }

                rapidjson::StringBuffer buffer;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
                response_doc->Accept(writer);
                
                publish(topic, buffer.GetString());
            }
            std::cout << "MQTT response thread shutting down." << std::endl;
        });
    }

    void publish(const std::string& topic, const std::string& payload) {
        try {
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            client.publish(pubmsg)->wait_for(std::chrono::seconds(2));
            std::cout << "Published response to topic: " << topic << std::endl;
        } catch (const mqtt::exception& exc) {
            std::cerr << "Error publishing to topic " << topic << ": " << exc.what() << std::endl;
        }
    }
};

// Internal callback handler class
class MqttCallbackHandler : public virtual mqtt::callback {
    MqttTransportImpl* p_impl;
    mqtt::async_client& client;

public:
    MqttCallbackHandler(MqttTransportImpl* impl, mqtt::async_client& cli) 
        : p_impl(impl), client(cli) {}

    void message_arrived(mqtt::const_message_ptr msg) override;
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
};

// MqttTransport public methods
MqttTransport::MqttTransport(std::shared_ptr<ObjectPool<RequestData>> pool, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue) 
    : p_impl(new MqttTransportImpl(MQTT_SERVER_ADDRESS, MQTT_CLIENT_ID, pool, queue)) {
    p_impl->callback = std::make_unique<MqttCallbackHandler>(p_impl, p_impl->client);
    p_impl->client.set_callback(*(p_impl->callback));
}

MqttTransport::~MqttTransport() {
    p_impl->stop_flag_.store(true);
    p_impl->responseChan->push(nullptr);
    if(p_impl->response_thread_.joinable()) {
        p_impl->response_thread_.join();
    }
    delete p_impl;
}

void MqttTransport::start() {
    try {
        std::cout << "Connecting to MQTT broker..." << std::endl;
        p_impl->client.connect(p_impl->connOpts)->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error connecting to MQTT broker: " << exc.what() << std::endl;
    }
}

void MqttTransport::stop() {
    try {
        p_impl->client.disconnect()->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error disconnecting: " << exc.what() << std::endl;
    }
}

// Callback implementations
void MqttCallbackHandler::message_arrived(mqtt::const_message_ptr msg) {
    std::cout << "MQTT message arrived on topic: " << msg->get_topic() << std::endl;

    rapidjson::Document payload_doc;
    payload_doc.Parse(msg->get_payload().c_str());

    if (payload_doc.HasParseError() || !payload_doc.IsObject() || !payload_doc.HasMember("topic") || !payload_doc["topic"].IsString() || !payload_doc.HasMember("request") || !payload_doc["request"].IsString()) {
        std::cerr << "MQTT message payload is not a valid JSON or has missing fields." << std::endl;
        return;
    }
    
    std::string response_topic = payload_doc["topic"].GetString();
    std::string request_str = payload_doc["request"].GetString();
    
    auto request_doc_ptr = JsonSchemaValidator::getInstance().validate(request_str);
    if (!request_doc_ptr) {
        // validate() already prints detailed errors
        return;
    }

    auto requestData = p_impl->pool->acquire();

    // requestId is technically optional in VISS but mandatory for our async handling
    if (!request_doc_ptr->HasMember("requestId") || !(*request_doc_ptr)["requestId"].IsString()) {
        std::cerr << "Bad Request: 'requestId' is missing or not a string in MQTT message." << std::endl;
        return; // Reject request without a requestId
    }
    requestData->requestId = (*request_doc_ptr)["requestId"].GetString();

    // Move the entire parsed document into the RequestData object
    requestData->request_doc->Swap(*request_doc_ptr);

    requestData->responseChan = p_impl->responseChan;
    {
        std::lock_guard<std::mutex> lock(p_impl->topic_map_mutex_);
        p_impl->request_topic_map_[requestData->requestId] = response_topic;
    }

    p_impl->requestQueue->push(requestData);
}

void MqttCallbackHandler::connected(const std::string& cause) {
    std::cout << "MQTT connection successful." << std::endl;
    std::cout << "Subscribing to topic: " << VISS_REQUEST_TOPIC << std::endl;
    client.subscribe(VISS_REQUEST_TOPIC, 1);
}

void MqttCallbackHandler::connection_lost(const std::string& cause) {
    std::cerr << "MQTT connection lost: " << cause << std::endl;
}
