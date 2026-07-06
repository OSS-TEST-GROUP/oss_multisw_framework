#ifndef VISSV2SERVER_H
#define VISSV2SERVER_H

#include <memory>
#include <vector>
#include <thread>
#include <string>
#include <map>
#include "ITransport.h"
#include "ServiceManager.h"
#include "ThreadSafeQueue.h"
#include "VssTree.h"
#include "ObjectPool.h"

// Main application class
class VissServerApp {
public:
    VissServerApp(const std::string& himPath);
    ~VissServerApp();
    void run();
    void stop();

private:
    std::atomic<bool> m_running{false};
    ThreadSafeQueue<int> m_shutdown_queue;
    std::shared_ptr<viss::VSSTree> vss_tree;
    std::shared_ptr<ServiceManager> service_manager;
    std::shared_ptr<ObjectPool<RequestData>> request_pool_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue;
    std::map<std::string, std::shared_ptr<ITransport>> transports;
    std::thread m_service_manager_thread;
};

#endif // VISSV2SERVER_H
