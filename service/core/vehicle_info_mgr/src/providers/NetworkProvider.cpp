#include "NetworkProvider.h"
#include "Logger.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

NetworkProvider::NetworkProvider() : active(false) {}

NetworkProvider::~NetworkProvider() {
    stop();
}

void NetworkProvider::start() {
    active = true;
    provider_thread = std::thread(&NetworkProvider::runLoop, this);
    LOG_INFO << "NetworkProvider started.";
}

void NetworkProvider::stop() {
    if (!active.exchange(false)) {
        return; // Already stopped
    }
    LOG_INFO << "NetworkProvider::stop() called.";
    if (provider_thread.joinable()) {
        provider_thread.join();
    }
    LOG_INFO << "NetworkProvider::stop() completed.";
}

std::string NetworkProvider::getName() const {
    return "NetworkProvider";
}

void NetworkProvider::setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) {
    data_channel = channel;
}

void NetworkProvider::setValue(const std::string& path, const std::any& value) {
    LOG_INFO << "NetworkProvider: setValue called for path " << path;
}

void NetworkProvider::setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) {
    vss_tree_ = vss_tree;
}

void NetworkProvider::runLoop() {
    while (active.load()) {
        if (data_channel) {
            // Original data points
            {
                VehicleData data;
                data.path = "Vehicle.IsMoving";
                data.value = (rand() % 2 == 0) ? "true" : "false";
                data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                data_channel->push(data);
            }

            // Generate data for the specified signals, converting all values to string.
            long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            
            data_channel->push(VehicleData{"Vehicle.CurrentLocation.Altitude", std::to_string(123.45 + (rand() % 10)), now_ms});
            data_channel->push(VehicleData{"Vehicle.CurrentLocation.Heading", std::to_string(90.0 + (rand() % 20 - 10)), now_ms});
            data_channel->push(VehicleData{"Vehicle.CurrentLocation.Latitude", std::to_string(37.422 + (double)(rand() % 1000) / 10000.0), now_ms});
            data_channel->push(VehicleData{"Vehicle.CurrentLocation.Longitude", std::to_string(-122.084 + (double)(rand() % 1000) / 10000.0), now_ms});
            data_channel->push(VehicleData{"Vehicle.Body.BodyType", std::string("Sedan"), now_ms});
            data_channel->push(VehicleData{"Vehicle.Body.Mirrors.DriverSide.IsFolded", (rand() % 5 == 0) ? "true" : "false", now_ms});
            data_channel->push(VehicleData{"Vehicle.Body.Mirrors.DriverSide.IsHeatingOn", (rand() % 10 == 0) ? "true" : "false", now_ms});
            data_channel->push(VehicleData{"Vehicle.Body.Mirrors.DriverSide.IsLocked", (rand() % 2 == 0) ? "true" : "false", now_ms});

            // Add tire pressure data
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.Pressure", std::to_string(32 + (rand() % 5 - 2)), now_ms});
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.IsPressureLow", (rand() % 20 == 0) ? "true" : "false", now_ms});
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.Pressure", std::to_string(32 + (rand() % 5 - 2)), now_ms});
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.IsPressureLow", (rand() % 20 == 0) ? "true" : "false", now_ms});
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.Pressure", std::to_string(34 + (rand() % 5 - 2)), now_ms});
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.IsPressureLow", (rand() % 20 == 0) ? "true" : "false", now_ms});
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.Pressure", std::to_string(34 + (rand() % 5 - 2)), now_ms});
            data_channel->push(VehicleData{"Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.IsPressureLow", (rand() % 20 == 0) ? "true" : "false", now_ms});
        }
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Slower updates
    }
}
