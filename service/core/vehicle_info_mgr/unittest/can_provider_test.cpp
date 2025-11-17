#include "gtest/gtest.h"
#include "CANProvider.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include <chrono>
#include <thread>

class CANProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        can_provider = std::make_unique<CANProvider>();
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
    }

    void TearDown() override {
        can_provider->stop();
    }

    std::unique_ptr<CANProvider> can_provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
};

TEST_F(CANProviderTest, StartAndStop) {
    can_provider->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    can_provider->stop();
    // No crash means success
}

TEST_F(CANProviderTest, DataIsPushedToChannel) {
    can_provider->setDataChannel(data_channel);
    can_provider->start();

    VehicleData data;
    bool result = data_channel->wait_and_pop(data);

    EXPECT_TRUE(result);
    EXPECT_EQ(data.path, "Vehicle.Speed");

    can_provider->stop();
}

TEST_F(CANProviderTest, SetValueDoesNotCrash) {
    can_provider->setValue("some/path", 42);
    // No crash means success
}
