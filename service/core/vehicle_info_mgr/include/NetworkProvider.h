#ifndef NETWORKPROVIDER_H
#define NETWORKPROVIDER_H

#include "IProvider.h"
#include <thread>
#include <atomic>

class NetworkProvider : public IProvider {
public:
    NetworkProvider();
    ~NetworkProvider() override;

    void start() override;
    void stop() override;
    std::string getName() const override;
    void setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) override;
    void setValue(const std::string& path, const std::any& value) override;
    void setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) override;

private:
    void runLoop();
    std::shared_ptr<viss::VSSTree> vss_tree_;

    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
    std::thread provider_thread;
    std::atomic<bool> active;
};

#endif // NETWORKPROVIDER_H
