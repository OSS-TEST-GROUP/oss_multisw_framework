#ifndef VISS_WEBSOCKET_TRANSPORT_H
#define VISS_WEBSOCKET_TRANSPORT_H

#include <libwebsockets.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <list>
#include <set>
#include "ITransport.h"
#include "ObjectPool.h"
#include "ThreadSafeQueue.h"

enum class SessionType {
    WEBSOCKET,
    HTTP,
    MQTT,
    UDS
};

// Structure to hold per-connection data
struct PerSessionData {
    bool initialized; // Track if C++ objects have been constructed (Placement New)
    struct lws *wsi;
    bool is_http;
    SessionType type;
    std::string http_uri;
    std::string http_method;
    std::string http_body;
};

class VissWebsocketTransport : public ITransport {
public:
    VissWebsocketTransport(std::shared_ptr<ObjectPool<RequestData>> pool, 
                           std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue);
    virtual ~VissWebsocketTransport();

    void start() override;
    void stop() override;

private:
    // LWS Callback
    static int callback_viss(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len);

    void run_event_loop();
    void response_handler_loop();

    // Internal methods for handling LWS events
    int handle_http(struct lws *wsi, void *in, size_t len);
    int handle_ws_receive(struct lws *wsi, void *in, size_t len);
    
    // Member variables
    std::shared_ptr<ObjectPool<RequestData>> m_pool;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> m_requestQueue;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>> m_responseChan;

    struct lws_context *m_context = nullptr;
    std::atomic<bool> m_stop_flag{false};
    std::thread m_event_loop_thread;
    std::thread m_response_thread;

    // Mapping for active connections to facilitate response delivery
    struct ClientSession {
        struct lws* wsi;
        SessionType type;                       // Track protocol type
        std::set<std::string> request_ids;      // Active requestId mappings
        std::set<std::string> subscription_ids; // Active subscriptionId mappings
    };

    std::mutex m_map_mutex;
    std::map<std::string, struct lws*> m_request_wsi_map;     // requestId -> wsi
    std::map<std::string, struct lws*> m_subscription_wsi_map; // subscriptionId -> wsi
    std::map<struct lws*, std::shared_ptr<ClientSession>> m_sessions; // wsi -> Session info
    
    // MQTT specific mappings
    std::map<std::string, std::string> m_mqtt_request_topic_map; // requestId -> response_topic
    std::map<std::string, std::string> m_mqtt_subscription_topic_map; // subscriptionId -> response_topic

    // Queue for responses that need to be sent via LWS (from response thread to LWS thread)
    struct OutgoingMessage {
        struct lws *wsi;
        std::string payload;
        SessionType type;
        int http_status;
        std::string mqtt_topic;
    };
    std::mutex m_out_queue_mutex;
    std::list<OutgoingMessage> m_out_queue;

    static const struct lws_protocols protocols[];
};

#endif // VISS_WEBSOCKET_TRANSPORT_H
