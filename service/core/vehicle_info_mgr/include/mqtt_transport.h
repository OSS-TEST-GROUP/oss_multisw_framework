#ifndef MQTT_TRANSPORT_H
#define MQTT_TRANSPORT_H

#include <memory>
#include "ITransport.h"
#include "ObjectPool.h"

// Forward declarations
struct MqttTransportImpl;

class MqttTransport : public ITransport {
    friend class MqttCallbackHandler;
public:
    MqttTransport(std::shared_ptr<ObjectPool<RequestData>> pool, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue);
    ~MqttTransport();
    
    void start() override;
    void stop() override;

private:
    MqttTransportImpl* p_impl;
};

#endif // MQTT_TRANSPORT_H
