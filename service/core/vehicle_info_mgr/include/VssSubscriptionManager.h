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
    struct Subscription {
        std::string subscriptionId;
        std::shared_ptr<RequestData> originalRequest;
        
        // The primary path that triggers the evaluation of this subscription.
        std::string triggerSignalPath;

        // All paths to watch for data changes for this subscription.
        std::vector<std::string> watchedPaths;

        // A list of filters to be applied to the trigger signal.
        std::vector<std::shared_ptr<viss::IFilter>> filters;

        // Stores the last known state of the signals to evaluate conditions (e.g., for 'change' filter).
        std::map<std::string, viss::DataPoint> lastState;

        std::atomic<bool> active{true};
    };

    VssSubscriptionManager(std::shared_ptr<IStorage> data_store, std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool, std::shared_ptr<viss::VSSTree> vss_tree);
    ~VssSubscriptionManager();

    std::string addSubscription(std::shared_ptr<RequestData> request);
    bool removeSubscription(const std::string& subscriptionId);
    
    void run();
    void stop();
    
private:
    std::map<std::string, std::shared_ptr<Subscription>> getAllSubscriptions();
    static std::vector<std::shared_ptr<viss::IFilter>> parseFilters(const rapidjson::Value& filter_json);

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
