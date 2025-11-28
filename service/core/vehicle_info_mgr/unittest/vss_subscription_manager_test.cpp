#include "gtest/gtest.h"
#include "VssSubscriptionManager.h"
#include "IStorage.h"
#include "VssTree.h"
#include "ObjectPool.h"
#include "Types.h"
#include "rapidjson/document.h"
#include <thread>
#include <chrono>

#include "ITransport.h"

class VssSubscriptionManagerTest : public ::testing::Test {
protected:
    std::optional<std::string> getEnv(const std::string& key) {
        const char* val = std::getenv(key.c_str());
        return val ? std::optional<std::string>(val) : std::nullopt;
    }

    void SetUp() override {
        data_store = std::make_shared<VssDataStore>();
        response_pool = std::make_shared<ObjectPool<PooledResponseMap>>();
        // The test must be run from the build directory.
        config_path = getEnv("VEHICLE_CONFIG_PATH").value_or("config");
        config_path.append("/viss.him");
        vss_tree = std::make_shared<viss::VSSTree>(config_path);
        if (!vss_tree->isValid()) {
            FAIL() << "Failed to load VSS tree from " << config_path;
        }
        sub_manager = std::make_shared<VssSubscriptionManager>(data_store, response_pool, vss_tree);
    }

    void TearDown() override {
        sub_manager->stop();
    }

    std::shared_ptr<IStorage> data_store;
    std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool;
    std::shared_ptr<viss::VSSTree> vss_tree;
    std::shared_ptr<VssSubscriptionManager> sub_manager;
	std::string config_path;

    void SetStoreValue(const std::string& path, const std::string& value) {
        VehicleData data;
        data.path = path;
        data.value = value;
        data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        data_store->setValue(data);
    }
};

TEST_F(VssSubscriptionManagerTest, AddAndRemoveSubscription) {
    auto request = std::make_shared<RequestData>();
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "requestId": "123"
    })");

    std::string sub_id = sub_manager->addSubscription(request);
    EXPECT_FALSE(sub_id.empty());

    bool removed = sub_manager->removeSubscription(sub_id);
    EXPECT_TRUE(removed);

    bool removed_again = sub_manager->removeSubscription(sub_id);
    EXPECT_FALSE(removed_again);
}

TEST_F(VssSubscriptionManagerTest, SubscriptionWithTimeFilter) {
    sub_manager->run();

    auto request = std::make_shared<RequestData>();
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "filter": [{"variant": "timebased", "parameter": {"period": "200"}}],
        "requestId": "test-time-filter"
    })");

    std::string sub_id = sub_manager->addSubscription(request);
    ASSERT_FALSE(sub_id.empty());
    
    std::shared_ptr<PooledResponseMap> response;

    // 1. Set a value, should trigger immediately.
    SetStoreValue("Vehicle.Speed", "10");
    EXPECT_TRUE(request->responseChan->wait_and_pop(response));

    // 2. Set another value right away, should NOT trigger.
    SetStoreValue("Vehicle.Speed", "20");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(request->responseChan->empty());

    // 3. Wait for the period to elapse.
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Total wait > 200ms

    // 4. Set another value, should trigger now.
    SetStoreValue("Vehicle.Speed", "30");
    EXPECT_TRUE(request->responseChan->wait_and_pop(response));

    sub_manager->removeSubscription(sub_id);
}

TEST_F(VssSubscriptionManagerTest, SubscriptionWithChangeFilter) {
    // Start the subscription manager's processing loop
    sub_manager->run();

    // 1. Subscribe with a "change" filter
    auto request = std::make_shared<RequestData>();
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "filter": [{"variant": "change"}],
        "requestId": "test-change-filter"
    })");

    std::string sub_id = sub_manager->addSubscription(request);
    ASSERT_FALSE(sub_id.empty());

    std::shared_ptr<PooledResponseMap> response;

    // 2. Set initial value - should trigger a notification
    SetStoreValue("Vehicle.Speed", "60");
    EXPECT_TRUE(request->responseChan->wait_and_pop(response));
    EXPECT_STREQ((*response)["subscriptionId"].GetString(), sub_id.c_str());

    // 3. Set the same value - should NOT trigger a notification
    SetStoreValue("Vehicle.Speed", "60");
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Allow time for processing
    EXPECT_TRUE(request->responseChan->empty());

    // 4. Set a different value - should trigger a notification
    SetStoreValue("Vehicle.Speed", "80");
    EXPECT_TRUE(request->responseChan->wait_and_pop(response));
    EXPECT_STREQ((*response)["subscriptionId"].GetString(), sub_id.c_str());

    sub_manager->removeSubscription(sub_id);
}
