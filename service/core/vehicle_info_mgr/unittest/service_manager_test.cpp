#include "gtest/gtest.h"
#include "ServiceManager.h"
#include "VssTree.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include "ITransport.h"
#include "rapidjson/document.h"
#include <chrono>
#include <thread>

class ServiceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        request_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
        vss_tree = std::make_shared<viss::VSSTree>("../config/viss.him");
        if (!vss_tree->isValid()) {
            FAIL() << "Failed to load VSS tree from ../config/viss.him";
        }
        service_manager = std::make_unique<ServiceManager>(request_queue, vss_tree);
        manager_thread = std::thread([this]() { service_manager->run(); });
    }

    void TearDown() override {
        service_manager->stop();
        if (manager_thread.joinable()) {
            manager_thread.join();
        }
    }

    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> request_queue;
    std::shared_ptr<viss::VSSTree> vss_tree;
    std::unique_ptr<ServiceManager> service_manager;
    std::thread manager_thread;
};

TEST_F(ServiceManagerTest, ProcessGetRequest) {
    // First, set a value to ensure there is something to get
    auto set_request = std::make_shared<RequestData>();
    set_request->requestId = "test-set-for-get";
    set_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    set_request->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "value": true,
        "requestId": "test-set-for-get"
    })");
    request_queue->push(set_request);
    
    std::shared_ptr<PooledResponseMap> set_response_ptr;
    set_request->responseChan->wait_and_pop(set_response_ptr);
    ASSERT_TRUE(set_response_ptr != nullptr);

    // Now, prepare and send the "get" request
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-get-01";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "requestId": "test-get-01"
    })");

    request_queue->push(get_request);

    // Wait for the response
    std::shared_ptr<PooledResponseMap> get_response_ptr;
    get_request->responseChan->wait_and_pop(get_response_ptr);
    ASSERT_TRUE(get_response_ptr != nullptr);
    auto& response_map = *get_response_ptr;

    // Check the response
    EXPECT_TRUE(response_map.HasMember("action"));
    EXPECT_STREQ(response_map["action"].GetString(), "get");
    EXPECT_TRUE(response_map.HasMember("requestId"));
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-get-01");
    ASSERT_TRUE(response_map.HasMember("data"));
    const auto& data = response_map["data"];
    ASSERT_TRUE(data.HasMember("dp"));
    const auto& dp = data["dp"];
    ASSERT_TRUE(dp.HasMember("value"));
    // The value is stored as a string "true"
    EXPECT_STREQ(dp["value"].GetString(), "true");
}

TEST_F(ServiceManagerTest, ProcessSetRequest) {
    // Prepare a "set" request for an actuator
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-set-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "value": true,
        "requestId": "test-set-01"
    })");

    request_queue->push(request);

    std::shared_ptr<PooledResponseMap> response_map_ptr;
    request->responseChan->wait_and_pop(response_map_ptr);
    ASSERT_TRUE(response_map_ptr != nullptr);
    auto& response_map = *response_map_ptr;

    EXPECT_TRUE(response_map.HasMember("action"));
    EXPECT_STREQ(response_map["action"].GetString(), "set");
    EXPECT_TRUE(response_map.HasMember("requestId"));
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-01");
    EXPECT_FALSE(response_map.HasMember("error"));

    // Now, let's verify with a "get" request
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-get-after-set";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "requestId": "test-get-after-set"
    })");

    request_queue->push(get_request);
    
    std::shared_ptr<PooledResponseMap> get_response_ptr;
    get_request->responseChan->wait_and_pop(get_response_ptr);
    ASSERT_TRUE(get_response_ptr != nullptr);
    auto& get_response = *get_response_ptr;

    ASSERT_TRUE(get_response.HasMember("data"));
    const auto& data = get_response["data"];
    ASSERT_TRUE(data.HasMember("dp"));
    const auto& dp = data["dp"];
    ASSERT_TRUE(dp.HasMember("value"));
    EXPECT_STREQ(dp["value"].GetString(), "true");
}
