#ifndef UDSPROVIDER_H
#define UDSPROVIDER_H

#include "IProvider.h"
#include <thread>
#include <atomic>
#include <string>

class UdsProvider : public IProvider {
public:
    UdsProvider();
    ~UdsProvider() override;

    void start() override;
    void stop() override;
    std::string getName() const override;
    void setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) override;
    void setValue(const std::string& path, const std::any& value) override;
    void setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) override;

private:
    void runLoop();
    void reconnectAndHandle();
    
    std::shared_ptr<viss::VSSTree> vss_tree_;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
    std::thread provider_thread;
    std::atomic<bool> active;
    std::string socket_path;
    int client_fd;
};

#endif // UDSPROVIDER_H
