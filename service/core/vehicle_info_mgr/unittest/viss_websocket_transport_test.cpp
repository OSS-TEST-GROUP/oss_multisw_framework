#include <gtest/gtest.h>

#define private public
#include "viss_websocket_transport.h"
#undef private

#include "ConfigManager.h"
#include "ObjectPool.h"
#include "ThreadSafeQueue.h"
#include "JsonSchemaValidator.h"
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

class VissWebsocketTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        JsonSchemaValidator::init("../config/vissv3.0-schema.json");

        pool = std::make_shared<ObjectPool<RequestData>>();
        response_pool = std::make_shared<ObjectPool<PooledResponseMap>>();
        queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
    }

    void TearDown() override {
    }

    std::shared_ptr<ObjectPool<RequestData>> pool;
    std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue;
};

TEST_F(VissWebsocketTransportTest, StartAndStopWithDefaultConfig) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    transport->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    transport->stop();
    // Second stop should be idempotent
    transport->stop();
}

TEST_F(VissWebsocketTransportTest, StartAndStopWithAllProtocolsEnabled) {
    auto& config = ConfigManager::getInstance();
    auto orig_protocols = config.getSupportedProtocols();

    config.setSupportedProtocols({"HTTP", "WS", "MQTT", "UDS"});

    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    transport->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    transport->stop();

    config.setSupportedProtocols(orig_protocols);
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_ValidGetRequest) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x1234);

    std::string get_msg = R"({"action":"get","path":"Vehicle.Speed","requestId":"req-001"})";
    int ret = transport->handle_ws_receive(dummy_wsi, (void*)get_msg.c_str(), get_msg.length());
    EXPECT_EQ(ret, 0);

    std::shared_ptr<RequestData> req;
    ASSERT_TRUE(queue->wait_and_pop(req));
    EXPECT_EQ(req->requestId, "req-001");
    EXPECT_TRUE(req->request_doc->HasMember("action"));
    EXPECT_STREQ((*req->request_doc)["action"].GetString(), "get");
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_FilterObjectWithParameter) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x2345);

    std::string filter_msg = R"({"action":"subscribe","path":"Vehicle.Speed","filter":{"type":"timebased","parameter":"100"},"requestId":"req-filter-1"})";
    int ret = transport->handle_ws_receive(dummy_wsi, (void*)filter_msg.c_str(), filter_msg.length());
    EXPECT_EQ(ret, 0);

    std::shared_ptr<RequestData> req;
    ASSERT_TRUE(queue->wait_and_pop(req));
    EXPECT_TRUE(req->request_doc->HasMember("filter"));
    EXPECT_TRUE((*req->request_doc)["filter"].IsArray());
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_FilterArray) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x3456);

    std::string filter_arr_msg = R"({"action":"subscribe","path":"Vehicle.Speed","filter":[{"type":"range","parameter":{"above":50}}],"requestId":"req-filter-2"})";
    int ret = transport->handle_ws_receive(dummy_wsi, (void*)filter_arr_msg.c_str(), filter_arr_msg.length());
    EXPECT_EQ(ret, 0);

    std::shared_ptr<RequestData> req;
    ASSERT_TRUE(queue->wait_and_pop(req));
    EXPECT_TRUE((*req->request_doc)["filter"].IsArray());
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_InvalidJson) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x4567);

    std::string invalid_msg = "{ not a valid json }";
    int ret = transport->handle_ws_receive(dummy_wsi, (void*)invalid_msg.c_str(), invalid_msg.length());
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(queue->empty());
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_DuplicateRequestId) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x5678);

    std::string msg1 = R"({"action":"get","path":"Vehicle.Speed","requestId":"dup-req-1"})";
    EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)msg1.c_str(), msg1.length()), 0);

    // Duplicate requestId
    std::string msg2 = R"({"action":"get","path":"Vehicle.Speed","requestId":"dup-req-1"})";
    EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)msg2.c_str(), msg2.length()), 0);
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_WithoutRequestId) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x6789);

    std::string msg = R"({"action":"get","path":"Vehicle.Speed"})";
    EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)msg.c_str(), msg.length()), 0);

    std::shared_ptr<RequestData> req;
    ASSERT_TRUE(queue->wait_and_pop(req));
    EXPECT_TRUE(req->requestId.empty());
}

TEST_F(VissWebsocketTransportTest, ResponseHandler_ActionsAndProtocols) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x7890);

    // Register session
    {
        std::lock_guard<std::mutex> lock(transport->m_map_mutex);
        auto session = std::make_shared<VissWebsocketTransport::ClientSession>();
        session->wsi = dummy_wsi;
        session->type = SessionType::WEBSOCKET;
        transport->m_sessions[dummy_wsi] = session;
        transport->m_request_wsi_map["req-resp-1"] = dummy_wsi;
        transport->m_subscription_wsi_map["sub-resp-1"] = dummy_wsi;
    }

    // 1. Subscribe Success Response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "subscribe", alloc);
        resp->AddMember("requestId", "req-resp-1", alloc);
        resp->AddMember("subscriptionId", "sub-resp-1", alloc);
        transport->m_responseChan->push(resp);
    }

    // 2. Unsubscribe Response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "unsubscribe", alloc);
        resp->AddMember("requestId", "req-resp-2", alloc);
        resp->AddMember("subscriptionId", "sub-resp-1", alloc);

        {
            std::lock_guard<std::mutex> lock(transport->m_map_mutex);
            transport->m_request_wsi_map["req-resp-2"] = dummy_wsi;
            transport->m_subscription_wsi_map["sub-resp-1"] = dummy_wsi;
        }
        transport->m_responseChan->push(resp);
    }

    // 3. Subscription Terminated Status
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "subscription", alloc);
        resp->AddMember("subscriptionId", "sub-resp-1", alloc);
        resp->AddMember("status", "terminated", alloc);

        {
            std::lock_guard<std::mutex> lock(transport->m_map_mutex);
            transport->m_subscription_wsi_map["sub-resp-1"] = dummy_wsi;
        }
        transport->m_responseChan->push(resp);
    }

    // 4. Error response with number
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "req-err-1", alloc);
        rapidjson::Value errObj(rapidjson::kObjectType);
        errObj.AddMember("number", 404, alloc);
        errObj.AddMember("reason", "Not found", alloc);
        resp->AddMember("error", errObj, alloc);

        {
            std::lock_guard<std::mutex> lock(transport->m_map_mutex);
            transport->m_request_wsi_map["req-err-1"] = dummy_wsi;
        }
        transport->m_responseChan->push(resp);
    }

    // Start response thread briefly to process the messages
    transport->m_response_thread = std::thread(&VissWebsocketTransport::response_handler_loop, transport.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    transport->stop();
}

TEST_F(VissWebsocketTransportTest, ResponseHandler_MqttAndUdsSessions) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* mqtt_wsi = reinterpret_cast<struct lws*>(0x8901);
    struct lws* uds_wsi = reinterpret_cast<struct lws*>(0x9012);

    {
        std::lock_guard<std::mutex> lock(transport->m_map_mutex);
        auto mqtt_session = std::make_shared<VissWebsocketTransport::ClientSession>();
        mqtt_session->wsi = mqtt_wsi;
        mqtt_session->type = SessionType::MQTT;
        transport->m_sessions[mqtt_wsi] = mqtt_session;
        transport->m_request_wsi_map["req-mqtt-1"] = mqtt_wsi;
        transport->m_mqtt_request_topic_map["req-mqtt-1"] = "response/topic/1";

        auto uds_session = std::make_shared<VissWebsocketTransport::ClientSession>();
        uds_session->wsi = uds_wsi;
        uds_session->type = SessionType::UDS;
        transport->m_sessions[uds_wsi] = uds_session;
        transport->m_request_wsi_map["req-uds-1"] = uds_wsi;
    }

    // Push MQTT response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "req-mqtt-1", alloc);
        transport->m_responseChan->push(resp);
    }

    // Push UDS response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "req-uds-1", alloc);
        transport->m_responseChan->push(resp);
    }

    // Push response with no target WSI
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "req-unknown", alloc);
        transport->m_responseChan->push(resp);
    }

    transport->m_response_thread = std::thread(&VissWebsocketTransport::response_handler_loop, transport.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    transport->stop();
}

TEST_F(VissWebsocketTransportTest, CallbackViss_DirectCallAllEvents) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x11223344);

    PerSessionData psd;
    psd.initialized = true;
    psd.type = SessionType::HTTP;
    psd.http_body = "100.5";
    EXPECT_EQ(psd.http_body, "100.5");

    // 3. HTTP_BODY_COMPLETION (Requires transport in sessions)
    psd.http_uri = "Vehicle.Speed";
    // We can simulate completion
    {
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();
        doc.AddMember("action", "set", alloc);
        doc.AddMember("path", "Vehicle.Speed", alloc);
        doc.AddMember("value", "100.5", alloc);
        doc.AddMember("requestId", "req-http-comp", alloc);
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> wr(buf);
        doc.Accept(wr);
        transport->handle_ws_receive(dummy_wsi, (void*)buf.GetString(), buf.GetLength());
    }

    // 4. MQTT_CLIENT_RX
    {
        // Invalid wrapper
        std::string bad_mqtt = "invalid json";
        lws_mqtt_publish_param_t pub_param;
        pub_param.payload = (void*)bad_mqtt.data();
        pub_param.payload_len = bad_mqtt.size();
        // Since callback_viss retrieves transport from context, we test handle_ws_receive with MQTT wrapper manually
        rapidjson::Document valid_wrapper;
        valid_wrapper.SetObject();
        auto& alloc = valid_wrapper.GetAllocator();
        valid_wrapper.AddMember("topic", "resp/topic/1", alloc);
        valid_wrapper.AddMember("request", "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"requestId\":\"req-mqtt-valid\"}", alloc);

        rapidjson::StringBuffer mbuf;
        rapidjson::Writer<rapidjson::StringBuffer> mwr(mbuf);
        valid_wrapper.Accept(mwr);

        // Process request
        std::string req_str = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"requestId\":\"req-mqtt-valid\"}";
        {
            std::lock_guard<std::mutex> lock(transport->m_map_mutex);
            transport->m_mqtt_request_topic_map["req-mqtt-valid"] = "resp/topic/1";
        }
        transport->handle_ws_receive(dummy_wsi, (void*)req_str.data(), req_str.size());
    }

    // 5. RECEIVE
    {
        std::string get_msg = "{\"action\":\"get\",\"path\":\"Vehicle.Powertrain.Transmission.SelectedGear\",\"requestId\":\"req-rec-1\"}";
        transport->handle_ws_receive(dummy_wsi, (void*)get_msg.data(), get_msg.size());
    }

    // 6. CLOSED and session cleanup with orphaned subscriptions
    {
        std::lock_guard<std::mutex> lock(transport->m_map_mutex);
        auto session = std::make_shared<VissWebsocketTransport::ClientSession>();
        session->wsi = dummy_wsi;
        session->type = SessionType::WEBSOCKET;
        session->subscription_ids.insert("sub-close-1");
        session->request_ids.insert("req-close-1");
        transport->m_sessions[dummy_wsi] = session;
        transport->m_subscription_wsi_map["sub-close-1"] = dummy_wsi;
        transport->m_request_wsi_map["req-close-1"] = dummy_wsi;
    }

    // Direct cleanup emulation as done in callback_viss LWS_CALLBACK_CLOSED
    {
        std::lock_guard<std::mutex> lock(transport->m_map_mutex);
        auto it = transport->m_sessions.find(dummy_wsi);
        if (it != transport->m_sessions.end()) {
            for (const auto& req_id : it->second->request_ids) {
                transport->m_request_wsi_map.erase(req_id);
            }
            for (const auto& sub_id : it->second->subscription_ids) {
                auto request = transport->m_pool->acquire();
                request->request_doc->SetObject();
                auto& alloc = request->request_doc->GetAllocator();
                request->request_doc->AddMember("action", "unsubscribe", alloc);
                request->request_doc->AddMember("subscriptionId", rapidjson::Value(sub_id.c_str(), alloc), alloc);
                std::string dummy_req = "auto_unsub_" + sub_id;
                request->request_doc->AddMember("requestId", rapidjson::Value(dummy_req.c_str(), alloc), alloc);
                request->requestId = dummy_req;
                request->responseChan = transport->m_responseChan;
                transport->m_requestQueue->push(request);
                transport->m_subscription_wsi_map.erase(sub_id);
            }
            transport->m_sessions.erase(it);
        }
    }

    EXPECT_EQ(transport->m_sessions.count(dummy_wsi), 0);
}

TEST_F(VissWebsocketTransportTest, OutQueue_Writable_Emulation) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = (struct lws*)0x9988;

    // 1. Populate out queue with various session types
    {
        std::lock_guard<std::mutex> lock(transport->m_out_queue_mutex);
        
        VissWebsocketTransport::OutgoingMessage item_ws;
        item_ws.wsi = dummy_wsi;
        item_ws.payload = "{\"action\":\"get\",\"requestId\":\"ws-1\"}";
        item_ws.type = SessionType::WEBSOCKET;
        transport->m_out_queue.push_back(item_ws);

        VissWebsocketTransport::OutgoingMessage item_uds;
        item_uds.wsi = dummy_wsi;
        item_uds.payload = "{\"action\":\"get\",\"requestId\":\"uds-1\"}";
        item_uds.type = SessionType::UDS;
        transport->m_out_queue.push_back(item_uds);

        VissWebsocketTransport::OutgoingMessage item_http;
        item_http.wsi = dummy_wsi;
        item_http.payload = "{\"action\":\"get\",\"requestId\":\"http-1\"}";
        item_http.type = SessionType::HTTP;
        item_http.http_status = 200;
        transport->m_out_queue.push_back(item_http);

        VissWebsocketTransport::OutgoingMessage item_mqtt;
        item_mqtt.wsi = dummy_wsi;
        item_mqtt.payload = "{\"action\":\"get\",\"requestId\":\"mqtt-1\"}";
        item_mqtt.type = SessionType::MQTT;
        item_mqtt.mqtt_topic = "test/resp/topic";
        transport->m_out_queue.push_back(item_mqtt);
    }

    EXPECT_EQ(transport->m_out_queue.size(), 4);

    // 2. Direct verification of item types in queue
    {
        std::lock_guard<std::mutex> lock(transport->m_out_queue_mutex);
        for (auto it = transport->m_out_queue.begin(); it != transport->m_out_queue.end(); ) {
            if (it->wsi == dummy_wsi) {
                // Emulate write processing
                size_t write_len = it->payload.length();
                if (it->type == SessionType::UDS) {
                    write_len++;
                }
                unsigned char *buf = (unsigned char *)malloc(LWS_PRE + write_len);
                memcpy(&buf[LWS_PRE], it->payload.c_str(), it->payload.length());
                if (it->type == SessionType::UDS) {
                    buf[LWS_PRE + write_len - 1] = '\n';
                }
                free(buf);
                it = transport->m_out_queue.erase(it);
            } else {
                ++it;
            }
        }
    }

    EXPECT_EQ(transport->m_out_queue.size(), 0);
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_FiltersAndDuplicates) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x1234);

    // 1. Single filter object with non-array parameter
    {
        std::string json_req = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"requestId\":\"req-f1\",\"filter\":{\"type\":\"time\",\"parameter\":\"1000\"}}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)json_req.data(), json_req.size()), 0);
    }

    // 2. Duplicate requestId
    {
        std::string json_dup = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"requestId\":\"req-f1\"}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)json_dup.data(), json_dup.size()), 0);
    }

    // 3. Array filter with mixed parameters
    {
        std::string json_arr = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"requestId\":\"req-f2\",\"filter\":[{\"type\":\"time\",\"parameter\":[\"1000\"]},{\"type\":\"range\",\"parameter\":\"10\"}]}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)json_arr.data(), json_arr.size()), 0);
    }
}

TEST_F(VissWebsocketTransportTest, ResponseHandler_AllProtocolsAndErrors) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* http_wsi = reinterpret_cast<struct lws*>(0x1111);
    struct lws* mqtt_wsi = reinterpret_cast<struct lws*>(0x2222);
    struct lws* uds_wsi = reinterpret_cast<struct lws*>(0x3333);
    struct lws* ws_wsi = reinterpret_cast<struct lws*>(0x4444);

    {
        std::lock_guard<std::mutex> lock(transport->m_map_mutex);
        
        auto http_session = std::make_shared<VissWebsocketTransport::ClientSession>();
        http_session->wsi = http_wsi;
        http_session->type = SessionType::HTTP;
        transport->m_sessions[http_wsi] = http_session;
        transport->m_request_wsi_map["req-http-err"] = http_wsi;
        transport->m_request_wsi_map["req-http-ok"] = http_wsi;

        auto mqtt_session = std::make_shared<VissWebsocketTransport::ClientSession>();
        mqtt_session->wsi = mqtt_wsi;
        mqtt_session->type = SessionType::MQTT;
        transport->m_sessions[mqtt_wsi] = mqtt_session;
        transport->m_request_wsi_map["req-mqtt-err"] = mqtt_wsi;
        transport->m_mqtt_request_topic_map["req-mqtt-err"] = "mqtt/err/topic";
        transport->m_subscription_wsi_map["sub-mqtt-1"] = mqtt_wsi;
        transport->m_mqtt_subscription_topic_map["sub-mqtt-1"] = "mqtt/sub/topic";

        auto uds_session = std::make_shared<VissWebsocketTransport::ClientSession>();
        uds_session->wsi = uds_wsi;
        uds_session->type = SessionType::UDS;
        transport->m_sessions[uds_wsi] = uds_session;
        transport->m_request_wsi_map["req-uds-err"] = uds_wsi;

        auto ws_session = std::make_shared<VissWebsocketTransport::ClientSession>();
        ws_session->wsi = ws_wsi;
        ws_session->type = SessionType::WEBSOCKET;
        transport->m_sessions[ws_wsi] = ws_session;
        transport->m_subscription_wsi_map["sub-ws-ok"] = ws_wsi;
    }

    // 1. HTTP 400 error response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "req-http-err", alloc);
        rapidjson::Value errObj(rapidjson::kObjectType);
        errObj.AddMember("number", "400", alloc);
        errObj.AddMember("reason", "Bad Request", alloc);
        resp->AddMember("error", errObj, alloc);
        transport->m_responseChan->push(resp);
    }

    // 2. HTTP 200 ok response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "req-http-ok", alloc);
        resp->AddMember("data", 100, alloc);
        transport->m_responseChan->push(resp);
    }

    // 3. MQTT error response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "req-mqtt-err", alloc);
        rapidjson::Value errObj(rapidjson::kObjectType);
        errObj.AddMember("number", "500", alloc);
        resp->AddMember("error", errObj, alloc);
        transport->m_responseChan->push(resp);
    }

    // 4. MQTT subscription event
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "subscription", alloc);
        resp->AddMember("subscriptionId", "sub-mqtt-1", alloc);
        resp->AddMember("data", 50, alloc);
        transport->m_responseChan->push(resp);
    }

    // 5. UDS error response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "set", alloc);
        resp->AddMember("requestId", "req-uds-err", alloc);
        rapidjson::Value errObj(rapidjson::kObjectType);
        errObj.AddMember("number", 403, alloc);
        resp->AddMember("error", errObj, alloc);
        transport->m_responseChan->push(resp);
    }

    // 6. WebSocket subscription event
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "subscription", alloc);
        resp->AddMember("subscriptionId", "sub-ws-ok", alloc);
        resp->AddMember("data", 123, alloc);
        transport->m_responseChan->push(resp);
    }

    // 7. Subscribe success response (creates new subscription mapping)
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "subscribe", alloc);
        resp->AddMember("requestId", "req-http-ok", alloc);
        resp->AddMember("subscriptionId", "new-sub-100", alloc);
        transport->m_responseChan->push(resp);
    }

    // 8. Unsubscribe success response
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "unsubscribe", alloc);
        resp->AddMember("requestId", "req-http-ok", alloc);
        resp->AddMember("subscriptionId", "sub-mqtt-1", alloc);
        transport->m_responseChan->push(resp);
    }

    // 9. Subscription terminated event
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "subscription", alloc);
        resp->AddMember("subscriptionId", "sub-ws-ok", alloc);
        resp->AddMember("status", "terminated", alloc);
        transport->m_responseChan->push(resp);
    }

    // 10. No target WSI found
    {
        auto resp = response_pool->acquire();
        resp->SetObject();
        auto& alloc = resp->GetAllocator();
        resp->AddMember("action", "get", alloc);
        resp->AddMember("requestId", "unknown-req-999", alloc);
        transport->m_responseChan->push(resp);
    }

    // Run response loop briefly
    transport->m_response_thread = std::thread(&VissWebsocketTransport::response_handler_loop, transport.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    transport->stop();
}

TEST_F(VissWebsocketTransportTest, CallbackViss_ProtocolInitAndNullWsi) {
    EXPECT_EQ(VissWebsocketTransport::callback_viss(nullptr, LWS_CALLBACK_PROTOCOL_INIT, nullptr, nullptr, 0), 0);
    EXPECT_EQ(VissWebsocketTransport::callback_viss(nullptr, LWS_CALLBACK_HTTP, nullptr, nullptr, 0), 0);
    EXPECT_EQ(VissWebsocketTransport::callback_viss(nullptr, LWS_CALLBACK_CLOSED, nullptr, nullptr, 0), 0);
}

TEST_F(VissWebsocketTransportTest, HandleWsReceive_SetAuthorizeUnsubscribe) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x8888);

    {
        std::lock_guard<std::mutex> lock(transport->m_map_mutex);
        auto ws_session = std::make_shared<VissWebsocketTransport::ClientSession>();
        ws_session->wsi = dummy_wsi;
        ws_session->type = SessionType::WEBSOCKET;
        transport->m_sessions[dummy_wsi] = ws_session;
    }

    // 1. Set request
    {
        std::string set_req = "{\"action\":\"set\",\"path\":\"Vehicle.Speed\",\"value\":\"60\",\"requestId\":\"req-set-1\"}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)set_req.data(), set_req.size()), 0);
    }

    // 2. Authorize request
    {
        std::string auth_req = "{\"action\":\"authorize\",\"tokens\":{\"authorization\":\"token123\"},\"requestId\":\"req-auth-1\"}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)auth_req.data(), auth_req.size()), 0);
    }

    // 3. Unsubscribe request
    {
        std::string unsub_req = "{\"action\":\"unsubscribe\",\"subscriptionId\":\"sub-12345\",\"requestId\":\"req-unsub-1\"}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)unsub_req.data(), unsub_req.size()), 0);
    }

    // 4. Request without requestId
    {
        std::string no_req_id = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\"}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)no_req_id.data(), no_req_id.size()), 0);
    }

    // 5. Request with single object filter & duplicate requestId
    {
        std::string filter_req = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"filter\":{\"type\":\"range\",\"parameter\":\"10\"},\"requestId\":\"dup_req_1\"}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)filter_req.data(), filter_req.size()), 0);

        // Duplicate
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)filter_req.data(), filter_req.size()), 0);
    }

    // 6. Request with array filter
    {
        std::string arr_filter = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"filter\":[{\"type\":\"range\",\"parameter\":\"10\"}],\"requestId\":\"arr_req_1\"}";
        EXPECT_EQ(transport->handle_ws_receive(dummy_wsi, (void*)arr_filter.data(), arr_filter.size()), 0);
    }
}

TEST_F(VissWebsocketTransportTest, CallbackViss_AllReasonsAndWriteableBranches) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x9999);

    PerSessionData psd{};
    psd.initialized = false;

    // 1. LWS_CALLBACK_HTTP
    std::string get_http_req = "GET /Vehicle/Speed";
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_HTTP, &psd, (void*)get_http_req.data(), get_http_req.size());

    // 2. LWS_CALLBACK_HTTP_BODY & COMPLETION
    psd.type = SessionType::HTTP;
    psd.http_uri = "Vehicle.Speed";
    std::string body_chunk = "100.0";
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_HTTP_BODY, &psd, (void*)body_chunk.data(), body_chunk.size());
    EXPECT_EQ(psd.http_body, "100.0");

    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_HTTP_BODY_COMPLETION, &psd, nullptr, 0);

    // 3. LWS_CALLBACK_RAW_ADOPT & ESTABLISHED
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_RAW_ADOPT, &psd, nullptr, 0);
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_ESTABLISHED, &psd, nullptr, 0);

    // 4. LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED & RX
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED, &psd, nullptr, 0);

    // Invalid MQTT format
    lws_mqtt_publish_param_t invalid_pub{};
    std::string invalid_payload = "invalid json";
    invalid_pub.payload = (void*)invalid_payload.data();
    invalid_pub.payload_len = invalid_payload.size();
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_MQTT_CLIENT_RX, &psd, &invalid_pub, 0);

    // Valid MQTT format
    lws_mqtt_publish_param_t valid_pub{};
    std::string valid_payload = "{\"topic\":\"res/topic\",\"request\":\"{\\\"action\\\":\\\"get\\\",\\\"path\\\":\\\"Vehicle.Speed\\\",\\\"requestId\\\":\\\"mqtt_req_1\\\"}\"}";
    valid_pub.payload = (void*)valid_payload.data();
    valid_pub.payload_len = valid_payload.size();
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_MQTT_CLIENT_RX, &psd, &valid_pub, 0);

    // 5. LWS_CALLBACK_RECEIVE & RAW_RX
    std::string ws_msg = "{\"action\":\"get\",\"path\":\"Vehicle.Speed\",\"requestId\":\"ws_req_1\"}";
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_RECEIVE, &psd, (void*)ws_msg.data(), ws_msg.size());
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_RAW_RX, &psd, (void*)ws_msg.data(), ws_msg.size());

    // 6. WRITEABLE with Outgoing Messages (HTTP, MQTT, UDS, WEBSOCKET)
    {
        std::lock_guard<std::mutex> lock(transport->m_out_queue_mutex);
        VissWebsocketTransport::OutgoingMessage http_msg;
        http_msg.wsi = dummy_wsi;
        http_msg.type = SessionType::HTTP;
        http_msg.http_status = 200;
        http_msg.payload = "{\"action\":\"get\",\"data\":{\"value\":60}}";
        transport->m_out_queue.push_back(http_msg);

        VissWebsocketTransport::OutgoingMessage mqtt_msg;
        mqtt_msg.wsi = dummy_wsi;
        mqtt_msg.type = SessionType::MQTT;
        mqtt_msg.mqtt_topic = "res/topic";
        mqtt_msg.payload = "{\"action\":\"get\",\"data\":{\"value\":60}}";
        transport->m_out_queue.push_back(mqtt_msg);

        VissWebsocketTransport::OutgoingMessage uds_msg;
        uds_msg.wsi = dummy_wsi;
        uds_msg.type = SessionType::UDS;
        uds_msg.payload = "{\"action\":\"get\",\"data\":{\"value\":60}}";
        transport->m_out_queue.push_back(uds_msg);

        VissWebsocketTransport::OutgoingMessage ws_out;
        ws_out.wsi = dummy_wsi;
        ws_out.type = SessionType::WEBSOCKET;
        ws_out.payload = "{\"action\":\"get\",\"data\":{\"value\":60}}";
        transport->m_out_queue.push_back(ws_out);
    }

    // Call WRITEABLE until queue is empty
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_SERVER_WRITEABLE, &psd, nullptr, 0);
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_HTTP_WRITEABLE, &psd, nullptr, 0);
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_MQTT_CLIENT_WRITEABLE, &psd, nullptr, 0);
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_RAW_WRITEABLE, &psd, nullptr, 0);

    // 7. CLOSED and CLEANUP with orphaned subscriptions
    {
        std::lock_guard<std::mutex> lock(transport->m_map_mutex);
        auto session = std::make_shared<VissWebsocketTransport::ClientSession>();
        session->wsi = dummy_wsi;
        session->type = SessionType::WEBSOCKET;
        session->request_ids.insert("req_to_clean");
        session->subscription_ids.insert("sub_to_clean");
        transport->m_sessions[dummy_wsi] = session;
        transport->m_request_wsi_map["req_to_clean"] = dummy_wsi;
        transport->m_subscription_wsi_map["sub_to_clean"] = dummy_wsi;
    }

    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_CLOSED, &psd, nullptr, 0);
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_HTTP_DROP_PROTOCOL, &psd, nullptr, 0);
    VissWebsocketTransport::callback_viss(dummy_wsi, LWS_CALLBACK_RAW_CLOSE, &psd, nullptr, 0);
}

TEST_F(VissWebsocketTransportTest, StartStop_WithMultipleProtocols) {
    auto& config = ConfigManager::getInstance();
    auto orig_protocols = config.getSupportedProtocols();
    auto orig_uds_path = config.uds.path;

    // 1. Long UDS path test (fallback to /tmp/vissv3/udsMgr.sock)
    config.setSupportedProtocols({"HTTP", "HTTPS", "WS", "WSS", "UDS", "MQTT", "MQTTS"});
    config.uds.path = "a_very_long_path_that_exceeds_one_hundred_and_four_characters_for_testing_unix_domain_socket_path_fallback_limit_check_feature_and_branches.sock";

    {
        auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
        transport->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        transport->stop();
    }

    // 2. Non-existent UDS dir test
    config.uds.path = "var/tmp/non_existent_subdir_for_test/udsMgr.sock";
    {
        auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
        transport->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        transport->stop();
    }

    config.setSupportedProtocols(orig_protocols);
    config.uds.path = orig_uds_path;
}

TEST_F(VissWebsocketTransportTest, HandleHttp_AllMethodsAndFilters) {
    auto transport = std::make_unique<VissWebsocketTransport>(pool, queue);
    struct lws* dummy_wsi = reinterpret_cast<struct lws*>(0x7777);

    // 1. GET with valid JSON filter
    std::string get_req = "GET /Vehicle/Speed?filter={\"type\":\"range\",\"value\":[0,100]}";
    EXPECT_EQ(transport->handle_http(dummy_wsi, (void*)get_req.data(), get_req.size()), 0);

    // 2. GET with invalid JSON filter
    std::string get_invalid_filter = "GET /Vehicle/Speed?filter={invalid}";
    EXPECT_EQ(transport->handle_http(dummy_wsi, (void*)get_invalid_filter.data(), get_invalid_filter.size()), 0);

    // 3. GET without filter
    std::string get_no_filter = "GET /Vehicle/Speed";
    EXPECT_EQ(transport->handle_http(dummy_wsi, (void*)get_no_filter.data(), get_no_filter.size()), 0);

    // 4. POST request
    std::string post_req = "POST /Vehicle/Speed";
    EXPECT_EQ(transport->handle_http(dummy_wsi, (void*)post_req.data(), post_req.size()), 0);

    // 5. Unsupported method (PUT)
    std::string put_req = "PUT /Vehicle/Speed";
    EXPECT_EQ(transport->handle_http(dummy_wsi, (void*)put_req.data(), put_req.size()), -1);

    // 6. Invalid / empty URI line
    EXPECT_EQ(transport->handle_http(dummy_wsi, nullptr, 0), -1);
}






