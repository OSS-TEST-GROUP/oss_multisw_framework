#include "CANProvider.h"
#include "Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

CANProvider::CANProvider() : active(false) {}

CANProvider::~CANProvider() {
    stop();
}

void CANProvider::start() {
    active = true;
    provider_thread = std::thread(&CANProvider::runLoop, this);
    LOG_INFO << "CANProvider started.";
}

void CANProvider::stop() {
    if (!active.exchange(false)) {
        return; // Already stopped
    }
    LOG_INFO << "CANProvider::stop() called.";
    if (provider_thread.joinable()) {
        provider_thread.join();
    }
    LOG_INFO << "CANProvider::stop() completed.";
}

std::string CANProvider::getName() const {
    return "CANProvider";
}

void CANProvider::setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) {
    data_channel = channel;
}

void CANProvider::setValue(const std::string& path, const std::any& value) {
    LOG_INFO << "CANProvider: setValue called for path " << path;
}

void CANProvider::setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) {
    // This provider does not need the VSS tree, so the implementation is empty.
}

void CANProvider::runLoop() {
    while (active.load()) {
        if (data_channel) {
            VehicleData data;
            data.path = "Vehicle.Speed";
            data.value = std::to_string(60 + (rand() % 20 - 10)); // 50 to 70 km/h
            data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            data_channel->push(data);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Faster updates
    }
}
