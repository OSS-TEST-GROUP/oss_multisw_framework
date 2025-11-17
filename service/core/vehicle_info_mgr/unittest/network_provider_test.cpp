#include "gtest/gtest.h"
#include "NetworkProvider.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include <chrono>
#include <thread>

class NetworkProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        network_provider = std::make_unique<NetworkProvider>();
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
    }

    void TearDown() override {
        network_provider->stop();
    }

    std::unique_ptr<NetworkProvider> network_provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
};

TEST_F(NetworkProviderTest, StartAndStop) {
    network_provider->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    network_provider->stop();
    // No crash means success
}

TEST_F(NetworkProviderTest, DataIsPushedToChannel) {
    network_provider->setDataChannel(data_channel);
    network_provider->start();

    VehicleData data;
    bool result = data_channel->wait_and_pop(data);

    EXPECT_TRUE(result);
    // We can't be sure which data point will be first, so just check that a path is not empty
    EXPECT_FALSE(data.path.empty());

    network_provider->stop();
}

TEST_F(NetworkProviderTest, SetValueDoesNotCrash) {
    network_provider->setValue("some/path", 42);
    // No crash means success
}
