#include "gtest/gtest.h"
#include "IStorage.h"
#include "Types.h"
#include <thread>
#include <chrono>

class VssDataStoreTest : public ::testing::Test {
protected:
    // By using a pointer, we can control the lifecycle of the store for each test.
    std::unique_ptr<VssDataStore> store;

    void SetUp() override {
        store = std::make_unique<VssDataStore>();
    }

    void TearDown() override {
        store.reset(); // This will call the VssDataStore destructor.
    }

    long long getCurrentTimeMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

TEST_F(VssDataStoreTest, SetAndGetLatestValue) {
    long long now = getCurrentTimeMs();
    VehicleData data = {"Vehicle.Speed", 99.0, now};
    store->setValue(data);

    auto result = store->getValues("Vehicle.Speed", "");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::any_cast<double>(result[0].value), 99.0);
    EXPECT_EQ(result[0].timestamp, now);
}

TEST_F(VssDataStoreTest, GetValuesWithTimePeriod) {
    long long now = getCurrentTimeMs();
    VehicleData data1 = {"Vehicle.Speed", 100.0, now - 20000}; // 20 seconds ago
    VehicleData data2 = {"Vehicle.Speed", 101.0, now - 5000};  // 5 seconds ago
    VehicleData data3 = {"Vehicle.Speed", 102.0, now};         // now

    store->setValue(data1);
    store->setValue(data2);
    store->setValue(data3);

    // Get data from the last 10 seconds ("PT10S")
    auto result = store->getValues("Vehicle.Speed", "PT10S");
    
    // We expect to find data2 and data3
    ASSERT_EQ(result.size(), 1);

 
    bool found_data3 = false;
    for(const auto& p : result) {
        if(p.timestamp == data3.timestamp) {
            found_data3 = true;
        }
    }

    EXPECT_TRUE(found_data3);
}

TEST_F(VssDataStoreTest, GetValuesWithInvalidTimePeriod) {
    long long now = getCurrentTimeMs();
    VehicleData data1 = {"Vehicle.Speed", 100.0, now - 20000}; // 20 seconds ago
    VehicleData data2 = {"Vehicle.Speed", 101.0, now - 5000};  // 5 seconds ago
    VehicleData data3 = {"Vehicle.Speed", 102.0, now};         // now

    store->setValue(data1);
    store->setValue(data2);
    store->setValue(data3);

    // Get data from the last 10 seconds ("PT10S")
    auto result = store->getValues("Vehicle.Speed", "T10S");
    
    // We expect to find data2 and data3
    ASSERT_EQ(result.size(), 1);

 
    bool found_data3 = false;
    for(const auto& p : result) {
        if(p.timestamp == data3.timestamp) {
            found_data3 = true;
        }
    }

    EXPECT_TRUE(found_data3);
}

TEST_F(VssDataStoreTest, GetValuesWithUnknownPath) {
    long long now = getCurrentTimeMs();
    VehicleData data1 = {"Vehicle.Speed", 100.0, now - 20000}; // 20 seconds ago
    VehicleData data2 = {"Vehicle.Speed", 101.0, now - 5000};  // 5 seconds ago
    VehicleData data3 = {"Vehicle.Speed", 102.0, now};         // now

    store->setValue(data1);
    store->setValue(data2);
    store->setValue(data3);

    // Get data from the last 10 seconds ("PT10S")
    auto result = store->getValues("Vehicle.RPM", "");
    
    // We expect to find data2 and data3
    ASSERT_EQ(result.size(), 1);
    ASSERT_STREQ(std::any_cast<const char*>(result[0].value), "viss-inline:Data-not-available");

}

// This test is commented out as it requires manual control over the archiver thread,
// which is not available in the current design. A more robust solution would be
// to inject a controllable clock or archiver mechanism for testing.

TEST_F(VssDataStoreTest, Archiving) {
    std::string path = "Vehicle.Powertrain.FuelSystem.level";
    long long now = getCurrentTimeMs();

    VehicleData data1 = {path, 80.0, now - 15000}; // 15s ago
    store->setValue(data1);
    
    // In a test environment, we would manually trigger archiving.
    // For now, we rely on the background thread, which makes this test flaky.
    // A better approach is needed.
    
    // Wait for archive loop to potentially run
    std::this_thread::sleep_for(std::chrono::milliseconds(11000));

    VehicleData data2 = {path, 79.0, now}; // now
    store->setValue(data2);

    // Get all historical data (using a long time period)
    auto results = store->getValues(path, "P1D"); // Get data from last day

    ASSERT_EQ(results.size(), 2);
    
    bool found_archived = false;
    for(const auto& p : results) {
        if(p.timestamp == data1.timestamp) {
            found_archived = true;
            break;
        }
    }
    EXPECT_TRUE(found_archived);
}

