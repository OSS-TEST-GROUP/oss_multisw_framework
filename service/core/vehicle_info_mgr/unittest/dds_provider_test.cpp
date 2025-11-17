#include "gtest/gtest.h"
#include "DDSProvider.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include <thread>
#include <chrono>

class DDSProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        dds_provider = std::make_unique<DDSProvider>();
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
    }

    void TearDown() override {
        dds_provider->stop();
    }

    std::unique_ptr<DDSProvider> dds_provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
};

TEST_F(DDSProviderTest, StartAndStop) {
    dds_provider->setDataChannel(data_channel);
    dds_provider->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    dds_provider->stop();
    // No crash means success
}

TEST_F(DDSProviderTest, SetValueDoesNotCrash) {
    dds_provider->setDataChannel(data_channel);
    dds_provider->start();
    dds_provider->setValue("Vehicle.Speed", 60.0);
    dds_provider->stop();
    // No crash means success
}
