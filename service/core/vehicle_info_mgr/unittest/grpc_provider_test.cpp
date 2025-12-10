#include "gtest/gtest.h"
#include "GrpcProvider.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include <thread>
#include <chrono>

class GrpcProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a non-existent server address for testing without a live server
        grpc_provider = std::make_unique<GrpcProvider>("localhost:50059");
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
    }

    void TearDown() override {
        grpc_provider->stop();
    }

    std::unique_ptr<GrpcProvider> grpc_provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
};

TEST_F(GrpcProviderTest, StartAndStop) {
    grpc_provider->setDataChannel(data_channel);
    grpc_provider->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    grpc_provider->stop();
    // No crash means success
}

TEST_F(GrpcProviderTest, SubscribeAndUnsubscribeWithoutServer) {
    grpc_provider->setDataChannel(data_channel);
    grpc_provider->start();
    grpc_provider->subscribe("Vehicle.Speed");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    grpc_provider->unsubscribe("Vehicle.Speed");
    grpc_provider->stop();
    // No crash means success
}
