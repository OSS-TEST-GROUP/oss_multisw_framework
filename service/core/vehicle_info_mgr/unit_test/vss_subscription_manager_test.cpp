#include "gtest/gtest.h"
#include "VssSubscriptionManager.h"
#include "IStorage.h"
#include "VssTree.h"
#include "ObjectPool.h"
#include "Types.h"
#include "rapidjson/document.h"

#include "ITransport.h"

class VssSubscriptionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        data_store = std::make_shared<VssDataStore>();
        response_pool = std::make_shared<ObjectPool<PooledResponseMap>>();
        // The test must be run from the build directory.
        vss_tree = std::make_shared<viss::VSSTree>("../config/viss.him");
        if (!vss_tree->isValid()) {
            FAIL() << "Failed to load VSS tree from ../config/viss.him";
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
    auto request = std::make_shared<RequestData>();
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "filter": {"type": "time", "value": 100},
        "requestId": "456"
    })");

    std::string sub_id = sub_manager->addSubscription(request);
    EXPECT_FALSE(sub_id.empty());

    // Here we would need to trigger the publisher loop and check responses.
    // This requires a more complex test setup with mock transport.
    // For now, we just check if the subscription was added.
    
    sub_manager->stop();
}
