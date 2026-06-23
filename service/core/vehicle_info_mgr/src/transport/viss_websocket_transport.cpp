#include "viss_websocket_transport.h"
#include "Logger.h"
#include "JsonSchemaValidator.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#include "ConfigManager.h"
#include <algorithm>
#include <new>

// VISS Protocol Protocols
const struct lws_protocols VissWebsocketTransport::protocols[] = {
    {
        "VISSv3",                     // Name (Sub-protocol)
        VissWebsocketTransport::callback_viss, // Callback
        sizeof(PerSessionData),       // Per session data size
        1024,                         // RX buffer size
    },
    { NULL, NULL, 0, 0 }             // Terminator
};

VissWebsocketTransport::VissWebsocketTransport(std::shared_ptr<ObjectPool<RequestData>> pool, 
                                               std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue)
    : m_pool(pool), m_requestQueue(queue), 
      m_responseChan(std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>()) {
}

VissWebsocketTransport::~VissWebsocketTransport() {
    stop();
}

void VissWebsocketTransport::start() {
    m_stop_flag = false;
    auto& config = ConfigManager::getInstance();
    const auto& supported = config.getSupportedProtocols();

    // 헬퍼 람다: 프로토콜 지원 여부 확인
    auto is_supported = [&](const std::string& p) {
        return std::find(supported.begin(), supported.end(), p) != supported.end();
    };

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.user = this;
    info.port = CONTEXT_PORT_NO_LISTEN; 

    m_context = lws_create_context(&info);
    if (!m_context) {
        LOG_ERROR << "Failed to create libwebsockets context.";
        return;
    }

    std::filesystem::path base_dir(config.getHimDirectory());

    // --- 1. Vhost for HTTPS ---
    if (is_supported("HTTPS")) {
        std::string full_cert = (base_dir / config.http.certPath).string();
        std::string full_key = (base_dir / config.http.keyPath).string();

        struct lws_context_creation_info vhost_info;
        memset(&vhost_info, 0, sizeof(vhost_info));
        vhost_info.port = config.http.port;
        vhost_info.protocols = protocols;
        vhost_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        vhost_info.ssl_cert_filepath = full_cert.c_str();
        vhost_info.ssl_private_key_filepath = full_key.c_str();
        if (lws_create_vhost(m_context, &vhost_info)) {
            LOG_INFO << "HTTPS vhost created on port " << vhost_info.port << " (Cert: " << full_cert << ")";
        } else {
            LOG_ERROR << "Failed to create HTTPS vhost on port " << vhost_info.port;
        }
    }

    // --- 2. Vhost for WSS ---
    if (is_supported("WSS")) {
        std::string full_cert = (base_dir / config.websocket.certPath).string();
        std::string full_key = (base_dir / config.websocket.keyPath).string();

        struct lws_context_creation_info vhost_info;
        memset(&vhost_info, 0, sizeof(vhost_info));
        vhost_info.port = config.websocket.port;
        vhost_info.protocols = protocols;
        vhost_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        vhost_info.ssl_cert_filepath = full_cert.c_str();
        vhost_info.ssl_private_key_filepath = full_key.c_str();
        if (lws_create_vhost(m_context, &vhost_info)) {
            LOG_INFO << "WSS vhost created on port " << vhost_info.port << " (Cert: " << full_cert << ")";
        } else {
            LOG_ERROR << "Failed to create WSS vhost on port " << vhost_info.port;
        }
    }

    // --- 3. Vhost for UDS (Unix Domain Socket) ---
    if (is_supported("UDS")) {
        std::string uds_path = (base_dir / config.uds.path).string();
        if (uds_path.length() >= 104) {
            uds_path = "/tmp/vissv3/udsMgr.sock";
            LOG_WARN << "[VissTransport] UDS path too long. Using fallback: " << uds_path;
        }
        std::string uds_dir = std::filesystem::path(uds_path).parent_path().string();
        try {
            if (!uds_dir.empty() && !std::filesystem::exists(uds_dir)) {
                std::filesystem::create_directories(uds_dir);
                chmod(uds_dir.c_str(), 0777);
            }
            if (std::filesystem::exists(uds_path)) {
                std::filesystem::remove(uds_path);
            }
            struct lws_context_creation_info vhost_info;
            memset(&vhost_info, 0, sizeof(vhost_info));
            vhost_info.iface = uds_path.c_str();
            vhost_info.port = 0; // For UDS, port must be 0
            vhost_info.protocols = protocols;
            vhost_info.options = LWS_SERVER_OPTION_UNIX_SOCK | LWS_SERVER_OPTION_FALLBACK_TO_RAW;
            if (lws_create_vhost(m_context, &vhost_info)) {
                LOG_INFO << "UDS vhost created on " << uds_path;
                chmod(uds_path.c_str(), 0777);
            } else {
                LOG_ERROR << "Failed to create UDS vhost on " << uds_path;
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "UDS Setup Error: " << e.what();
        }
    }

    // --- 4. MQTT Client Connection ---
    if (is_supported("MQTTS")) {
        struct lws_client_connect_info i;
        memset(&i, 0, sizeof(i));
        i.context = m_context;
        i.address = "localhost"; // 브로커 주소
        i.port = config.mqtt.port;
        i.path = "/";
        i.host = i.address;
        i.origin = i.address;
        i.protocol = "mqtt";
        i.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
        
        lws_mqtt_client_connect_param_t cp;
        memset(&cp, 0, sizeof(cp));
        cp.client_id = "viss_server_mqtt_transport";
        cp.keep_alive = 60;
        cp.clean_start = 1;
        i.mqtt_cp = &cp;

        if (lws_client_connect_via_info(&i)) {
            LOG_INFO << "MQTT client connection initiated on port " << i.port;
        } else {
            LOG_ERROR << "Failed to initiate MQTT client connection.";
        }
    }

    // --- 5. 모든 설정 완료 후 쓰레드 시작 ---
    m_event_loop_thread = std::thread(&VissWebsocketTransport::run_event_loop, this);
    m_response_thread = std::thread(&VissWebsocketTransport::response_handler_loop, this);
    
    LOG_INFO << "VissWebsocketTransport started with all supported protocols.";
}

void VissWebsocketTransport::stop() {
    m_stop_flag = true;
    
    // Wake up LWS
    if (m_context) {
        lws_cancel_service(m_context);
    }
    
    // Wake up response thread
    m_responseChan->push(nullptr);

    if (m_event_loop_thread.joinable()) {
        m_event_loop_thread.join();
    }
    if (m_response_thread.joinable()) {
        m_response_thread.join();
    }

    if (m_context) {
        lws_context_destroy(m_context);
        m_context = nullptr;
    }
    
    LOG_INFO << "VissWebsocketTransport stopped.";
}

void VissWebsocketTransport::run_event_loop() {
    int n = 0;
    while (!m_stop_flag && n >= 0) {
        n = lws_service(m_context, 100);
    }
    LOG_INFO << "LWS event loop finished.";
}

void VissWebsocketTransport::response_handler_loop() {
    LOG_INFO << "[VissWebsocketTransport] Response handler loop started.";
    while (!m_stop_flag) {
        std::shared_ptr<PooledResponseMap> response;
        m_responseChan->wait_and_pop(response);

        if (!response || m_stop_flag) break;

        // 1. Extract basic information from the response
        std::string reqId, subId, action, status;
        bool is_error = response->HasMember("error");
        int status_code = 200;

        if (response->HasMember("requestId") && (*response)["requestId"].IsString()) {
            reqId = (*response)["requestId"].GetString();
        }
        if (response->HasMember("subscriptionId") && (*response)["subscriptionId"].IsString()) {
            subId = (*response)["subscriptionId"].GetString();
        }
        if (response->HasMember("action") && (*response)["action"].IsString()) {
            action = (*response)["action"].GetString();
        }

        LOG_INFO << "[VissWebsocketTransport] Handling response: action=" << action << ", reqId=" << reqId << ", subId=" << subId;

        // Handle error status code if present
        if (is_error && (*response)["error"].IsObject()) {
            const auto& errorObj = (*response)["error"];
            if (errorObj.HasMember("number")) {
                if (errorObj["number"].IsInt()) status_code = errorObj["number"].GetInt();
                else if (errorObj["number"].IsString()) status_code = std::stoi(errorObj["number"].GetString());
            }
            LOG_INFO << "[VissWebsocketTransport] Error response detected. status_code=" << status_code;
        }

        struct lws* target_wsi = nullptr;
        std::string mqtt_topic;
        SessionType type = SessionType::WEBSOCKET;

        // 2. Action-based state management and WSI lookup
        {
            std::lock_guard<std::mutex> lock(m_map_mutex);

            // Find destination WSI
            if (!subId.empty() && m_subscription_wsi_map.count(subId)) {
                target_wsi = m_subscription_wsi_map[subId];
            } else if (!reqId.empty() && m_request_wsi_map.count(reqId)) {
                target_wsi = m_request_wsi_map[reqId];
            }

            if (!target_wsi) {
                LOG_WARN << "No target WSI found for response. action=" << action << ", reqId=" << reqId << ", subId=" << subId;
                continue;
            }

            if (m_sessions.count(target_wsi)) {
                type = m_sessions[target_wsi]->type;
                LOG_INFO << "[VissWebsocketTransport] Target WSI found. type=" << (int)type;
            }

            // MQTT Topic lookup
            if (type == SessionType::MQTT) {
                if (!subId.empty() && m_mqtt_subscription_topic_map.count(subId)) {
                    mqtt_topic = m_mqtt_subscription_topic_map[subId];
                } else if (!reqId.empty() && m_mqtt_request_topic_map.count(reqId)) {
                    mqtt_topic = m_mqtt_request_topic_map[reqId];
                }
                LOG_INFO << "[VissWebsocketTransport] MQTT topic resolved: " << mqtt_topic;
            }

            // Logic by Action
            if (action == "subscribe") {
                if (!is_error && !subId.empty()) {
                    // Success: Register subscription
                    m_subscription_wsi_map[subId] = target_wsi;
                    if (m_sessions.count(target_wsi)) {
                        m_sessions[target_wsi]->subscription_ids.insert(subId);
                    }
                    if (type == SessionType::MQTT && !mqtt_topic.empty()) {
                        m_mqtt_subscription_topic_map[subId] = mqtt_topic;
                    }
                    LOG_INFO << "[VissWebsocketTransport] Subscription registered: " << subId;
                }
            } 
            else if (action == "unsubscribe") {
                if (!is_error && !subId.empty()) {
                    // Success: Remove subscription
                    m_subscription_wsi_map.erase(subId);
                    m_mqtt_subscription_topic_map.erase(subId);
                    if (m_sessions.count(target_wsi)) {
                        m_sessions[target_wsi]->subscription_ids.erase(subId);
                    }
                    // For non-WebSocket, remove subscriptionId from client response as per VISS spec
                    response->EraseMember("subscriptionId");
                    
                    LOG_INFO << "[VissWebsocketTransport] Subscription removed: " << subId;
                }
            } 
            else if (action == "subscription") {
                if (response->HasMember("status") && (*response)["status"].IsString()) {
                    status = (*response)["status"].GetString();
                }

                if (status == "terminated") {
                    LOG_INFO << "[VissWebsocketTransport] Subscription terminated for subId: " << subId;
                    m_subscription_wsi_map.erase(subId);
                    m_mqtt_subscription_topic_map.erase(subId);
                    if (m_sessions.count(target_wsi)) {
                        m_sessions[target_wsi]->subscription_ids.erase(subId);
                    }
                    continue; // Traditionally termination signal is not sent to client
                }
            }

            // Clean up old request mappings (get, set, subscribe response, etc.)
            if (!reqId.empty()) {
                m_request_wsi_map.erase(reqId);
                m_mqtt_request_topic_map.erase(reqId);
                if (m_sessions.count(target_wsi)) {
                    m_sessions[target_wsi]->request_ids.erase(reqId);
                }
            }
        }

        // 3. Prepare and send the message
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response->Accept(writer);
        std::string payload = buffer.GetString();

        struct OutgoingMessage msg;
        msg.wsi = target_wsi;
        msg.payload = payload;
        msg.type = type;
        msg.http_status = status_code;
        msg.mqtt_topic = mqtt_topic;
        
        LOG_INFO << "[VissWebsocketTransport] Queueing outgoing message for wsi=" << target_wsi << ", payload=" << payload.substr(0, 50) << "...";

        {
            std::lock_guard<std::mutex> out_lock(m_out_queue_mutex);
            m_out_queue.push_back(msg);
        }
        LOG_INFO << "[VissTransport] msg pushed to out_queue.";
        
        if (target_wsi) {
            LOG_INFO << "[VissTransport] Calling lws_callback_on_writable...";
            lws_callback_on_writable(target_wsi);
            
            // LOG_INFO << "[VissTransport] Calling lws_cancel_service...";
            // auto ctx = lws_get_context(target_wsi);
            // if (ctx) {
            //     lws_cancel_service(ctx);
            // } else {
            //     LOG_ERROR << "[VissTransport] lws_get_context returned NULL!";
            // }
        }
        
        LOG_INFO << "[VissTransport] send_response function finished for wsi=" << target_wsi;
    }
}


int VissWebsocketTransport::callback_viss(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    struct lws_context *ctx = lws_get_context(wsi);
    VissWebsocketTransport *transport = (VissWebsocketTransport *)lws_context_user(ctx);
    PerSessionData *psd = (PerSessionData *)user;

    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
            // Set user data in context to retrieve 'this'
            // Wait, lws_get_vhost_user might be better, but we need the instance.
            // Actually, we can use info.user when creating the context. Correcting 'start' below.
            break;

        case LWS_CALLBACK_HTTP: {
            if (psd && !psd->initialized) {
                new(psd) PerSessionData(); 
                psd->initialized = true;
                LOG_INFO << "[VissTransport] PerSessionData initialized for HTTP.";
            }
            LOG_INFO << "[VissTransport] HTTP request received.";
            if (psd) {
                psd->wsi = wsi;
                psd->type = SessionType::HTTP;
                psd->http_body.clear();
            }
            if (transport) {
                // Initial session for HTTP (might be transient)
                std::lock_guard<std::mutex> lock(transport->m_map_mutex);
                auto session = std::make_shared<ClientSession>();
                session->wsi = wsi;
                session->type = SessionType::HTTP;
                transport->m_sessions[wsi] = session;
            }
            return transport ? transport->handle_http(wsi, in, len) : 0;
        }

        case LWS_CALLBACK_HTTP_BODY:
            if (psd && psd->type == SessionType::HTTP) {
                psd->http_body.append((const char *)in, len);
            }
            break;

        case LWS_CALLBACK_HTTP_BODY_COMPLETION:
            if (psd && psd->type == SessionType::HTTP && transport) {
                // Finalize POST request (VISS SET)
                rapidjson::Document doc;
                doc.SetObject();
                auto& allocator = doc.GetAllocator();
                doc.AddMember("action", "set", allocator);
                doc.AddMember("path", rapidjson::Value(psd->http_uri.c_str(), allocator), allocator);
                
                // Assuming body is a simple value as per user request
                doc.AddMember("value", rapidjson::Value(psd->http_body.c_str(), allocator), allocator);
                doc.AddMember("requestId", rapidjson::Value(std::to_string(std::uintptr_t(wsi)).c_str(), allocator), allocator);
                
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);
                
                transport->handle_ws_receive(wsi, (void*)buffer.GetString(), buffer.GetLength());
            }
            break;

        case LWS_CALLBACK_RAW_ADOPT:
            LOG_INFO << "[VissTransport] LWS_CALLBACK_RAW_ADOPT for wsi=" << wsi;
            // fallthrough
        case LWS_CALLBACK_ESTABLISHED: {
            if (psd && !psd->initialized) {
                new(psd) PerSessionData(); 
                psd->initialized = true;
                LOG_INFO << "[VissTransport] PerSessionData initialized for WebSocket/UDS.";
            }
            LOG_INFO << "[VissTransport] New connection established.";
            SessionType type = SessionType::WEBSOCKET;
            
            // Identify if this is a UDS connection by checking the vhost name
            struct lws_vhost* vh = lws_get_vhost(wsi);
            const char* vhost_name = lws_get_vhost_name(vh);
            if (vhost_name && strcmp(vhost_name, "uds") == 0) {
                type = SessionType::UDS;
                LOG_INFO << "UDS session identified.";
            }

            if (transport) {
                std::lock_guard<std::mutex> lock(transport->m_map_mutex);
                auto session = std::make_shared<ClientSession>();
                session->wsi = wsi;
                session->type = type;
                transport->m_sessions[wsi] = session;
            }
            psd->wsi = wsi;
            psd->type = type;
            break;
        }

        case LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED: {
            LOG_INFO << "MQTT client connection established.";
            if (transport) {
                std::lock_guard<std::mutex> lock(transport->m_map_mutex);
                auto session = std::make_shared<ClientSession>();
                session->wsi = wsi;
                session->type = SessionType::MQTT;
                transport->m_sessions[wsi] = session;
            }
            psd->wsi = wsi;
            psd->type = SessionType::MQTT;

            // Subscribe to VISS/request/#
            lws_mqtt_subscribe_param_t sub;
            lws_mqtt_topic_elem_t topics[1];
            memset(&sub, 0, sizeof(sub));
            topics[0].name = "VISS/request/#";
            topics[0].qos = QOS1;
            sub.num_topics = 1;
            sub.topic = topics;
            
            lws_mqtt_client_send_subcribe(wsi, &sub);
            break;
        }

        case LWS_CALLBACK_MQTT_CLIENT_RX: {
            lws_mqtt_publish_param_t *pub = (lws_mqtt_publish_param_t *)in;
            std::string message((const char *)pub->payload, pub->payload_len);
            
            LOG_INFO << "MQTT message received: " << message;
            
            // Parse MQTT wrapper: {"topic": "response_topic", "request": "viss_rpc_string"}
            rapidjson::Document wrapper;
            wrapper.Parse(message.c_str());
            if (wrapper.HasParseError() || !wrapper.IsObject() || !wrapper.HasMember("topic") || !wrapper.HasMember("request")) {
                LOG_ERROR << "Invalid MQTT VISS wrapper format.";
                return 0;
            }
            
            std::string response_topic = wrapper["topic"].GetString();
            std::string request_str = wrapper["request"].GetString();
            
            // Re-use handle_ws_receive login for the inner request
            if (transport) {
                // We need to map the requestId to the response_topic before handle_ws_receive parses it
                // Parsing twice is not ideal but let's do a quick peek for requestId
                rapidjson::Document inner;
                inner.Parse(request_str.c_str());
                if (!inner.HasParseError() && inner.HasMember("requestId") && inner["requestId"].IsString()) {
                    std::lock_guard<std::mutex> lock(transport->m_map_mutex);
                    transport->m_mqtt_request_topic_map[inner["requestId"].GetString()] = response_topic;
                }
                
                transport->handle_ws_receive(wsi, (void*)request_str.c_str(), request_str.length());
            }
            break;
        }

        case LWS_CALLBACK_RAW_CLOSE:
            LOG_INFO << "[VissTransport] LWS_CALLBACK_RAW_CLOSE for wsi=" << wsi;
            // fallthrough
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_HTTP_DROP_PROTOCOL: {
            std::string reason_str = (reason == LWS_CALLBACK_CLOSED) ? "LWS_CALLBACK_CLOSED" : "LWS_CALLBACK_HTTP_DROP_PROTOCOL";
            LOG_INFO << "[VissTransport] Connection closed (reason: " << reason_str << ") for wsi=" << wsi;
            
            if (psd && psd->initialized) {
                psd->~PerSessionData(); // Explicitly call destructor
                psd->initialized = false;
            }
            if (transport) {
                std::lock_guard<std::mutex> lock(transport->m_map_mutex);
                auto it = transport->m_sessions.find(wsi);
                if (it != transport->m_sessions.end()) {
                    // Clean up all requestId mappings
                    for (const auto& req_id : it->second->request_ids) {
                        transport->m_request_wsi_map.erase(req_id);
                    }
                    // Clean up all subscriptionId mappings and send unsubscribe to the core
                    for (const auto& sub_id : it->second->subscription_ids) {
                        // 1. Send automatic unsubscribe request to core
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
                        LOG_INFO << "[VissTransport] Sent auto-unsubscribe for orphaned sub_id: " << sub_id;

                        // 2. Clean up local mapping
                        transport->m_subscription_wsi_map.erase(sub_id);
                    }
                    transport->m_sessions.erase(it);
                }
            }
            break;
        }

        case LWS_CALLBACK_RAW_RX:
            LOG_INFO << "[VissTransport] LWS_CALLBACK_RAW_RX for wsi=" << wsi;
            // fallthrough
        case LWS_CALLBACK_RECEIVE: {
            LOG_INFO << "WebSocket/Raw message received.";
            return transport ? transport->handle_ws_receive(wsi, in, len) : 0;
        }

        case LWS_CALLBACK_RAW_WRITEABLE:
            LOG_INFO << "[VissTransport] LWS_CALLBACK_RAW_WRITEABLE for wsi=" << wsi;
            // fallthrough
        case LWS_CALLBACK_HTTP_WRITEABLE:
        case LWS_CALLBACK_MQTT_CLIENT_WRITEABLE:
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            LOG_INFO << "[VissTransport] WRITEABLE triggered (reason=" << reason << ") for wsi=" << wsi;
            if (!transport) {
                LOG_ERROR << "[VissTransport] transport instance is null in WRITEABLE!";
                return 0;
            }
            
            std::lock_guard<std::mutex> lock(transport->m_out_queue_mutex);
            auto it = transport->m_out_queue.begin();
            while (it != transport->m_out_queue.end()) {
                if (it->wsi == wsi) {
                    LOG_INFO << "[VissTransport] Sending message for wsi=" << wsi << ", type=" << (int)it->type;
                    size_t write_len = it->payload.length();
                    if (it->type == SessionType::UDS) {
                        write_len++; // Space for newline
                    }

                    unsigned char *buf = (unsigned char *)malloc(LWS_PRE + write_len);
                    memcpy(&buf[LWS_PRE], it->payload.c_str(), it->payload.length());
                    
                    if (it->type == SessionType::UDS) {
                        buf[LWS_PRE + write_len - 1] = '\n'; 
                    }

                    if (it->type == SessionType::HTTP) {
                        LOG_INFO << "[VissTransport] Writing HTTP Response: status=" << it->http_status << ", len=" << it->payload.length();
                        unsigned char hbuf[LWS_PRE + 2048];
                        unsigned char *p = &hbuf[LWS_PRE], *end = &hbuf[sizeof(hbuf) - 1];
                        
                        if (lws_add_http_common_headers(wsi, it->http_status, "application/json",
                                                        it->payload.length(), &p, end)) {
                            free(buf);
                            break; 
                        }
                        if (lws_finalize_http_header(wsi, &p, end)) {
                            free(buf);
                            break;
                        }
                        lws_write(wsi, &hbuf[LWS_PRE], p - &hbuf[LWS_PRE], LWS_WRITE_HTTP_HEADERS);
                        lws_write(wsi, &buf[LWS_PRE], it->payload.length(), LWS_WRITE_HTTP);
                        (void)lws_http_transaction_completed(wsi);
                        
                        free(buf);
                        it = transport->m_out_queue.erase(it);
                        break; // Finish for this writable callback to stay safe with HTTP state
                    } else if (it->type == SessionType::MQTT) {
                        LOG_INFO << "[VissTransport] Writing MQTT Publish: topic=" << it->mqtt_topic << ", len=" << it->payload.length();
                        lws_mqtt_publish_param_t pub;
                        memset(&pub, 0, sizeof(pub));
                        pub.topic = (char*)it->mqtt_topic.c_str();
                        pub.topic_len = it->mqtt_topic.length();
                        pub.payload_len = it->payload.length();
                        pub.qos = QOS1;
                        pub.retain = 0;
                        
                        lws_mqtt_client_send_publish(wsi, &pub, &buf[LWS_PRE], it->payload.length(), 1);
                        
                        free(buf);
                        it = transport->m_out_queue.erase(it);
                    } else {
                        enum lws_write_protocol write_mode = (it->type == SessionType::UDS) ? LWS_WRITE_RAW : LWS_WRITE_TEXT;
                        LOG_INFO << "[VissTransport] Writing TEXT/RAW: type=" << (int)it->type << ", len=" << write_len << ", mode=" << write_mode;
                        lws_write(wsi, &buf[LWS_PRE], write_len, write_mode);
                        
                        free(buf);
                        it = transport->m_out_queue.erase(it);
                    }
                } else {
                    ++it; // Move to next message if not for this WSI
                }
            }
            break;
        }

        default:
            break;
    }

    return 0;
}

int VissWebsocketTransport::handle_http(struct lws *wsi, void *in, size_t len) {
    LOG_INFO << "handle_http " << len;
    char uri[256];
    char *puri_ptr = NULL;
    int puri_len = 0;
    int method = lws_http_get_uri_and_method(wsi, &puri_ptr, &puri_len);

    if (method < 0 || puri_ptr == NULL || puri_len >= (int)sizeof(uri)) {
        return -1;
    }
    memcpy(uri, puri_ptr, puri_len);
    uri[puri_len] = '\0';
    
    std::string path_str(uri);
    // Convert /Vehicle/Speed to Vehicle.Speed
    if (!path_str.empty() && path_str[0] == '/') {
        path_str.erase(0, 1);
    }
    for (auto &c : path_str) {
        if (c == '/') c = '.';
    }

    LOG_INFO << "HTTP Request: Method=" << method << ", URI=" << path_str;

    if (method == LWSHUMETH_GET) {
        // Construct a VISS GET JSON-RPC request from HTTP GET
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();
        doc.AddMember("action", "get", allocator);
        doc.AddMember("path", rapidjson::Value(path_str.c_str(), allocator), allocator);
        doc.AddMember("requestId", rapidjson::Value(std::to_string(std::uintptr_t(wsi)).c_str(), allocator), allocator);
        
        // Handle filter query parameter
        char filter_arg[2048];
        if (lws_get_urlarg_by_name(wsi, "filter", filter_arg, sizeof(filter_arg)) != NULL) {
            rapidjson::Document filter_doc;
            filter_doc.Parse(filter_arg);
            if (!filter_doc.HasParseError()) {
                rapidjson::Value filter_val;
                filter_val.CopyFrom(filter_doc, allocator);
                doc.AddMember("filter", filter_val, allocator);
            } else {
                LOG_WARN << "Failed to parse filter query parameter as JSON: " << filter_arg;
            }
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        
        return handle_ws_receive(wsi, (void*)buffer.GetString(), buffer.GetLength());
    } else if (method == LWSHUMETH_POST) {
        // POST needs to wait for body callbacks
        // Store URI and wait
        PerSessionData *psd = (PerSessionData *)lws_wsi_user(wsi);
        if (psd) {
            psd->http_uri = path_str;
            psd->http_method = "POST";
        }
        return 0; 
    }

    return -1; 
}

int VissWebsocketTransport::handle_ws_receive(struct lws *wsi, void *in, size_t len) {
    std::string message((const char *)in, len);
    LOG_INFO << "Received message: " << message;

    // Parse JSON
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError()) {
        LOG_ERROR << "Failed to parse VISS JSON-RPC message: " << rapidjson::GetParseError_En(doc.GetParseError());
        return 0;
    }

    // Ensure the filter is always an array, and its parameter is also an array for ServiceManager
    if (doc.HasMember("filter")) {
        auto& allocator = doc.GetAllocator();
        rapidjson::Value& filters = doc["filter"];
        
        auto process_single_filter = [&](rapidjson::Value& filter_obj) {
            if (filter_obj.IsObject() && filter_obj.HasMember("parameter") && !filter_obj["parameter"].IsArray()) {
                rapidjson::Value param_val(filter_obj["parameter"], allocator);
                filter_obj.RemoveMember("parameter");
                rapidjson::Value param_array(rapidjson::kArrayType);
                param_array.PushBack(param_val, allocator);
                filter_obj.AddMember("parameter", param_array, allocator);
            }
        };

        if (filters.IsObject()) {
            process_single_filter(filters);
            rapidjson::Value filter_array(rapidjson::kArrayType);
            filter_array.PushBack(filters, allocator);
            doc.RemoveMember("filter");
            doc.AddMember("filter", filter_array, allocator);
        } else if (filters.IsArray()) {
            for (auto& f : filters.GetArray()) {
                process_single_filter(f);
            }
        }
    }

    // Create RequestData
    auto request = m_pool->acquire();
    request->request_doc->CopyFrom(doc, request->request_doc->GetAllocator());
    request->responseChan = m_responseChan; // Link back to our handler

    if (doc.HasMember("requestId") && doc["requestId"].IsString()) {
        std::string reqId = doc["requestId"].GetString();
        
        std::lock_guard<std::mutex> lock(m_map_mutex);
        if (m_request_wsi_map.count(reqId)) {
            LOG_ERROR << "Duplicate requestId received: " << reqId;
            // For now, just discard or we could send an error response here
            return 0; 
        }
        
        m_request_wsi_map[reqId] = wsi;
        if (m_sessions.count(wsi)) {
            m_sessions[wsi]->request_ids.insert(reqId);
        }
        request->requestId = reqId;
    }

    // Push to core queue
    m_requestQueue->push(request);

    return 0;
}
