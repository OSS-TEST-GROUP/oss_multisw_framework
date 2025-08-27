#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <any>

#include "IProvider.h"
#include "IStorage.h"
#include "ITransport.h"
#include "Types.h"
#include "ThreadSafeQueue.h"
#include "VssTree.h"
#include "ObjectPool.h"
#include "VssSubscriptionManager.h" // Include the new header
#include "VISSv3.0.grpc.pb.h"

#include <iostream>


// Handles the core service logic and manages data providers
class ServiceManager {
public:
    ServiceManager(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> request_queue, std::shared_ptr<viss::VSSTree> vss_tree);
    ~ServiceManager();

    void addProvider(std::unique_ptr<IProvider> provider);
    void run();
    void stop();
    std::shared_ptr<VssSubscriptionManager> getSubscriptionManager();

private:
    void processRequest(std::shared_ptr<RequestData> request);
    void dataChannelLoop();

    std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool_;
    std::shared_ptr<viss::VSSTree> vss_tree_;
    std::shared_ptr<IStorage> storage;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> dataChannel;
    std::shared_ptr<VssSubscriptionManager> subscription_manager;
    std::vector<std::unique_ptr<IProvider>> providers;
    std::atomic<bool> active{false};
    std::thread data_channel_thread;
};


#endif // SERVICEMANAGER_H
