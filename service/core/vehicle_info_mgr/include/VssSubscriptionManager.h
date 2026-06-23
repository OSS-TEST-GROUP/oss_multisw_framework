#ifndef VSS_SUBSCRIPTION_MANAGER_H
#define VSS_SUBSCRIPTION_MANAGER_H

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <any>

#include "IStorage.h"
#include "ITransport.h"
#include "Types.h"
#include "Filter.h"
#include "ThreadSafeQueue.h"
#include "VssTree.h"
#include "ObjectPool.h"

#include <iostream>

// Manages client subscriptions
class VssSubscriptionManager {
public:
    class Subscription; // Forward declaration

    VssSubscriptionManager(std::shared_ptr<IStorage> data_store, std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool, std::shared_ptr<viss::VSSTree> vss_tree);
    ~VssSubscriptionManager();

    std::string addSubscription(std::shared_ptr<RequestData> request);
    bool removeSubscription(const std::string& subscriptionId);
    
    void run();
    void stop();

    class Subscription {
    public:
        Subscription(
            std::string id,
            std::shared_ptr<RequestData> request,
            std::shared_ptr<viss::VSSTree> vss_tree,
            std::shared_ptr<IStorage> data_store,
            std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool
        );
        ~Subscription();

        void process();
        bool isActive() const;
        const std::string& getTriggerPath() const;
        const std::string& getSubscriptionId() const;
        void stop();

    private:
        static std::vector<std::shared_ptr<viss::IFilter>> parseFilters(const rapidjson::Value& filter_json);

        std::string m_subscriptionId;
        std::shared_ptr<RequestData> m_originalRequest;
        std::string m_triggerSignalPath;
        std::vector<std::string> m_watchedPaths;
        std::vector<std::shared_ptr<viss::IFilter>> m_filters;
        std::map<std::string, viss::DataPoint> m_lastState;
        std::atomic<bool> m_active{true};

        // Dependencies
        std::shared_ptr<IStorage> m_data_store;
        std::shared_ptr<ObjectPool<PooledResponseMap>> m_response_pool;
        
        friend class VssSubscriptionManager;
    };
    
private:
    long nextSubscriptionId = 1;
    // Holds all active subscriptions, keyed by subscriptionId.
    std::map<std::string, std::shared_ptr<Subscription>> m_subscriptions;
    // Maps a signal path to all subscriptions that are triggered by it.
    std::map<std::string, std::vector<std::shared_ptr<Subscription>>> m_triggerMap;

    std::mutex mtx;
    std::shared_ptr<IStorage> data_store;
    std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool_;
    std::shared_ptr<viss::VSSTree> vss_tree_;

    std::atomic<bool> active_{false};
    std::thread publisher_thread_;
};

#endif // VSS_SUBSCRIPTION_MANAGER_H
