#include "gtest/gtest.h"
#include "IStorage.h"
#include "Types.h"
#include <thread>
#include <chrono>

class VssDataStoreTest : public ::testing::Test {
protected:
    VssDataStore store;

    long long getCurrentTimeMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

TEST_F(VssDataStoreTest, SetAndGetLatestValue) {
    long long now = getCurrentTimeMs();
    VehicleData data = {"Vehicle.Speed", 99.0, now};
    store.setValue(data);

    auto result = store.getValues("Vehicle.Speed", "");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::any_cast<double>(result[0].value), 99.0);
    EXPECT_EQ(result[0].timestamp, now);
}

TEST_F(VssDataStoreTest, GetValuesWithTimePeriod) {
    long long now = getCurrentTimeMs();
    VehicleData data1 = {"Vehicle.Speed", 100.0, now - 20000}; // 20 seconds ago
    VehicleData data2 = {"Vehicle.Speed", 101.0, now - 5000};  // 5 seconds ago
    VehicleData data3 = {"Vehicle.Speed", 102.0, now};         // now

    store.setValue(data1);
    store.setValue(data2);
    store.setValue(data3);

    // This part of test is tricky because of the archive loop.
    // For now, we are only testing latest values.
    // Let's assume archiving hasn't happened yet for data2 and data3.

    // Get data from the last 10 seconds ("PT10S")
    auto result = store.getValues("Vehicle.Speed", "PT10S");
    // Depending on timing, this could be tricky.
    // The latest value should always be there if it's within the window.
    bool found_data3 = false;
    for(const auto& p : result) {
        if(p.timestamp == data3.timestamp) {
            found_data3 = true;
        }
    }
    EXPECT_TRUE(found_data3);
}

TEST_F(VssDataStoreTest, Archiving) {
    std::string path = "Vehicle.Powertrain.FuelSystem.level";
    long long now = getCurrentTimeMs();

    VehicleData data1 = {path, 80.0, now - 15000}; // 15s ago
    store.setValue(data1);
    
    // Wait for archive loop to run (default is 10s)
    std::this_thread::sleep_for(std::chrono::seconds(11));

    VehicleData data2 = {path, 79.0, now}; // now
    store.setValue(data2);

    // Get all historical data (using a long time period)
    auto results = store.getValues(path, "P1D"); // Get data from last day

    EXPECT_GE(results.size(), 1); 
    
    bool found_archived = false;
    for(const auto& p : results) {
        if(p.timestamp == data1.timestamp) {
            found_archived = true;
            break;
        }
    }
    // This might fail if the timing of the test is unlucky with the archive thread.
    // But we expect it to be archived.
    EXPECT_TRUE(found_archived);
}
