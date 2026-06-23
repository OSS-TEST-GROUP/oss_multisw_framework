#ifndef IVISS_CLIENT_H
#define IVISS_CLIENT_H

#include <string>
#include <functional>
#include <memory>

class IVissClient {
public:
    virtual ~IVissClient() = default;
    
    virtual void start(const std::string& address) = 0;
    
    virtual void stop() = 0;
    
    virtual void sendRequest(const std::string& jsonReq) = 0;
    
    using MessageCallback = std::function<void(const std::string&)>;
    using SubscriptionCallback = std::function<void(const std::string& path, const std::string& subId)>;
    
    void setResponseCallback(MessageCallback cb) { m_onResponse = cb; }
    void setEventCallback(MessageCallback cb) { m_onEvent = cb; }
    void setSubscriptionCallback(SubscriptionCallback cb) { m_onSubscription = cb; }

protected:
    MessageCallback m_onResponse;
    MessageCallback m_onEvent;
    SubscriptionCallback m_onSubscription;
};

#endif // IVISS_CLIENT_H
