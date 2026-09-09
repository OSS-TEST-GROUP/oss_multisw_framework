#include "gtest/gtest.h"
#include "ServiceManager.h"
#include "VssTree.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include "ITransport.h"
#include "rapidjson/document.h"
#include <chrono>
#include <thread>
#include <optional>

class ServiceManagerTest : public ::testing::Test {
protected:
    std::optional<std::string> getEnv(const std::string& key) {
        const char* val = std::getenv(key.c_str());
        return val ? std::optional<std::string>(val) : std::nullopt;
    }
    void SetUp() override {
        request_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
        config_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../config");
        config_path.append("/viss.him");
        vss_tree = std::make_shared<viss::VSSTree>(config_path);
        if (!vss_tree->isValid()) {
            FAIL() << "Failed to load VSS tree from " << config_path;
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
    std::string config_path;
};

TEST_F(ServiceManagerTest, ProcessGetRequest) {
    // First, set a value to ensure there is something to get
    auto set_request = std::make_shared<RequestData>();
    set_request->requestId = "test-set-for-get";
    set_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    set_request->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "value": "true",
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


TEST_F(ServiceManagerTest, ProcessGetRequestEmptyArrayFilter) {
    // First, set a value to ensure there is something to get
    auto set_request = std::make_shared<RequestData>();
    set_request->requestId = "test-set-for-get";
    set_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    set_request->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "value": "true",
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
        "filter":[],
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

TEST_F(ServiceManagerTest, ProcessGetRequestWithMetaFilter) {
    // First, set a value to ensure there is something to get
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-set-for-get";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.Speed",
        "filter":[{"variant":"metadata", "parameter":["1"]}],
        "requestId": "test-set-for-get"
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
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-for-get");
    ASSERT_TRUE(response_map.HasMember("metadata"));
    //const auto& data = response_map["data"];
    //ASSERT_TRUE(data.HasMember("dp"));
    //const auto& dp = data["dp"];
    //ASSERT_TRUE(dp.HasMember("value"));
    // The value is stored as a string "true"
    //EXPECT_STREQ(dp["value"].GetString(), "true");
}

TEST_F(ServiceManagerTest, ProcessGetRequestWithPathsAndMetaFilter) {
    // First, set a value to ensure there is something to get
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-set-for-get";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle",
        "filter":[{"variant":"paths", "parameter":["Speed","Height"]},{"variant":"metadata", "parameter":["1"]}],
        "requestId": "test-set-for-get"
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
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-for-get");
    ASSERT_TRUE(response_map.HasMember("metadata"));
    ASSERT_TRUE(response_map["metadata"].IsArray());
    ASSERT_EQ(response_map["metadata"].Size(), 2);
}

TEST_F(ServiceManagerTest, ProcessGetRequestWithMetaFilterAndInvalidPath) {
    // First, set a value to ensure there is something to get
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-set-for-get";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.Invalid.Path",
        "filter":[{"variant":"metadata", "parameter":["1"]}],
        "requestId": "test-set-for-get"
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
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-for-get");
    ASSERT_TRUE(response_map.HasMember("error"));
}

TEST_F(ServiceManagerTest, ProcessGetRequestWithMetaFilterAndInvalidParam) {
    // First, set a value to ensure there is something to get
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-set-for-get";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.Speed",
        "filter":[{"variant":"metadata", "parameter":["invalid_argument"]}],
        "requestId": "test-set-for-get"
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
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-for-get");
    ASSERT_TRUE(response_map.HasMember("metadata"));
}


TEST_F(ServiceManagerTest, ProcessGetRequestWithInvalidPath) {
    // First, set a value to ensure there is something to get
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-set-for-get";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.InvalidPath",
        "filter":[{"variant":"metadata", "parameter":["1"]}],
        "requestId": "test-set-for-get"
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
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-for-get");
    ASSERT_TRUE(response_map.HasMember("error"));
    //const auto& data = response_map["data"];
    //ASSERT_TRUE(data.HasMember("dp"));
    //const auto& dp = data["dp"];
    //ASSERT_TRUE(dp.HasMember("value"));
    // The value is stored as a string "true"
    //EXPECT_STREQ(dp["value"].GetString(), "true");
}

TEST_F(ServiceManagerTest, ProcessGetRequestWithHistoryFilter) {
    // First, set a value to ensure there is something to get
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-set-for-get";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.Height",
        "filter":[{"variant":"history", "parameter":["P2DT12H"]}],
        "requestId": "test-set-for-get"
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
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-for-get");
    ASSERT_TRUE(response_map.HasMember("data"));

    EXPECT_STREQ(response_map["data"]["dp"]["value"].GetString(), "viss-inline:Data-not-available");
    
    //const auto& data = response_map["data"];
    //ASSERT_TRUE(data.HasMember("dp"));
    //const auto& dp = data["dp"];
    //ASSERT_TRUE(dp.HasMember("value"));
    // The value is stored as a string "true"
    //EXPECT_STREQ(dp["value"].GetString(), "true");
}

TEST_F(ServiceManagerTest, ProcessGetRequestWithPathsFilter) {
    // First, set a value to ensure there is something to get
    auto get_request = std::make_shared<RequestData>();
    get_request->requestId = "test-set-for-get";
    get_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle",
        "filter":[{"variant":"paths", "parameter":["Speed","Height"]}],
        "requestId": "test-set-for-get"
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
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-for-get");
    ASSERT_TRUE(response_map.HasMember("data"));
    ASSERT_TRUE(response_map["data"].IsArray());
    ASSERT_EQ(response_map["data"].Size(), 2);
}


TEST_F(ServiceManagerTest, ProcessSetRequest) {
    // Prepare a "set" request for an actuator
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-set-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "value": "true",
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

TEST_F(ServiceManagerTest, ProcessSetRequestInvalidPath) {
    // Prepare a "set" request for an actuator
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-set-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.Invalid.Path",
        "value": "true",
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
    EXPECT_TRUE(response_map.HasMember("error"));
}

TEST_F(ServiceManagerTest, ProcessSetRequestNotActuator) {
    // Prepare a "set" request for an actuator
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-set-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.Speed",
        "value": "60",
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
    EXPECT_TRUE(response_map.HasMember("error"));
}

TEST_F(ServiceManagerTest, ProcessSubscribeRequest) {
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-subscribe-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "requestId": "test-subscribe-01"
    })");

    request_queue->push(request);

    std::shared_ptr<PooledResponseMap> response_map_ptr;
    request->responseChan->wait_and_pop(response_map_ptr);
    ASSERT_TRUE(response_map_ptr != nullptr);
    auto& response_map = *response_map_ptr;

    EXPECT_TRUE(response_map.HasMember("action"));
    EXPECT_STREQ(response_map["action"].GetString(), "subscribe");
    EXPECT_TRUE(response_map.HasMember("requestId"));
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-subscribe-01");
    EXPECT_TRUE(response_map.HasMember("subscriptionId"));
    EXPECT_FALSE(response_map["subscriptionId"].GetStringLength() == 0);
}

TEST_F(ServiceManagerTest, ProcessSubscribeRequestWithInvalidPath) {
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-subscribe-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Invalid.Path",
        "requestId": "test-subscribe-01"
    })");

    request_queue->push(request);

    std::shared_ptr<PooledResponseMap> response_map_ptr;
    request->responseChan->wait_and_pop(response_map_ptr);
    ASSERT_TRUE(response_map_ptr != nullptr);
    auto& response_map = *response_map_ptr;

    EXPECT_TRUE(response_map.HasMember("action"));
    EXPECT_STREQ(response_map["action"].GetString(), "subscribe");
    EXPECT_TRUE(response_map.HasMember("requestId"));
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-subscribe-01");
    EXPECT_TRUE(response_map.HasMember("error"));
}

TEST_F(ServiceManagerTest, ProcessSubscribeRequestWithUnsupportFilter) {
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-subscribe-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle",
        "filter": [{"variant":"paths", "parameter":["Speed","Height"]},{"variant":"metadata", "parameter":["1"]}],
        "requestId": "test-subscribe-01"
    })");

    request_queue->push(request);

    std::shared_ptr<PooledResponseMap> response_map_ptr;
    request->responseChan->wait_and_pop(response_map_ptr);
    ASSERT_TRUE(response_map_ptr != nullptr);
    auto& response_map = *response_map_ptr;

    EXPECT_TRUE(response_map.HasMember("action"));
    EXPECT_STREQ(response_map["action"].GetString(), "subscribe");
    EXPECT_TRUE(response_map.HasMember("requestId"));
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-subscribe-01");
    EXPECT_TRUE(response_map.HasMember("error"));
}

TEST_F(ServiceManagerTest, ProcessUnsubscribeRequest) {
    // First, subscribe to a path
    auto sub_request = std::make_shared<RequestData>();
    sub_request->requestId = "test-sub-for-unsub";
    sub_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    sub_request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "requestId": "test-sub-for-unsub"
    })");
    request_queue->push(sub_request);

    std::shared_ptr<PooledResponseMap> sub_response_ptr;
    sub_request->responseChan->wait_and_pop(sub_response_ptr);
    ASSERT_TRUE(sub_response_ptr != nullptr);
    auto& sub_response = *sub_response_ptr;
    std::string subscriptionId = sub_response["subscriptionId"].GetString();

    // Now, unsubscribe
    auto unsub_request = std::make_shared<RequestData>();
    unsub_request->requestId = "test-unsub-01";
    unsub_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    unsub_request->request_doc->Parse(
        (std::string(R"({
            "action": "unsubscribe",
            "subscriptionId": ")") + subscriptionId + R"(",
            "requestId": "test-unsub-01"
        })").c_str()
    );
    request_queue->push(unsub_request);

    std::shared_ptr<PooledResponseMap> unsub_response_ptr;
    unsub_request->responseChan->wait_and_pop(unsub_response_ptr);
    ASSERT_TRUE(unsub_response_ptr != nullptr);
    auto& unsub_response = *unsub_response_ptr;

    EXPECT_TRUE(unsub_response.HasMember("action"));
    EXPECT_STREQ(unsub_response["action"].GetString(), "unsubscribe");
    EXPECT_TRUE(unsub_response.HasMember("requestId"));
    EXPECT_STREQ(unsub_response["requestId"].GetString(), "test-unsub-01");
    EXPECT_TRUE(unsub_response.HasMember("subscriptionId"));
    EXPECT_STREQ(unsub_response["subscriptionId"].GetString(), subscriptionId.c_str());
}

TEST_F(ServiceManagerTest, ProcessUnsubscribeRequestWithIntID) {
    // First, subscribe to a path
    auto sub_request = std::make_shared<RequestData>();
    sub_request->requestId = "test-sub-for-unsub";
    sub_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    sub_request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "requestId": "test-sub-for-unsub"
    })");
    request_queue->push(sub_request);

    std::shared_ptr<PooledResponseMap> sub_response_ptr;
    sub_request->responseChan->wait_and_pop(sub_response_ptr);
    ASSERT_TRUE(sub_response_ptr != nullptr);
    auto& sub_response = *sub_response_ptr;
    std::string subscriptionId = sub_response["subscriptionId"].GetString();

    // Now, unsubscribe
    auto unsub_request = std::make_shared<RequestData>();
    unsub_request->requestId = "test-unsub-01";
    unsub_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    unsub_request->request_doc->Parse(
        (std::string(R"({
            "action": "unsubscribe",
            "subscriptionId": )") + subscriptionId + R"(,
            "requestId": "test-unsub-01"
        })").c_str()
    );
    request_queue->push(unsub_request);

    std::shared_ptr<PooledResponseMap> unsub_response_ptr;
    unsub_request->responseChan->wait_and_pop(unsub_response_ptr);
    ASSERT_TRUE(unsub_response_ptr != nullptr);
    auto& unsub_response = *unsub_response_ptr;

    EXPECT_TRUE(unsub_response.HasMember("action"));
    EXPECT_STREQ(unsub_response["action"].GetString(), "unsubscribe");
    EXPECT_TRUE(unsub_response.HasMember("error"));
    EXPECT_TRUE(unsub_response.HasMember("requestId"));
    EXPECT_STREQ(unsub_response["requestId"].GetString(), "test-unsub-01");
}

TEST_F(ServiceManagerTest, ProcessUnsubscribeRequestWithUnknownID) {
    // First, subscribe to a path
    auto sub_request = std::make_shared<RequestData>();
    sub_request->requestId = "test-sub-for-unsub";
    sub_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    sub_request->request_doc->Parse(R"({
        "action": "subscribe",
        "path": "Vehicle.Speed",
        "requestId": "test-sub-for-unsub"
    })");
    request_queue->push(sub_request);

    std::shared_ptr<PooledResponseMap> sub_response_ptr;
    sub_request->responseChan->wait_and_pop(sub_response_ptr);
    ASSERT_TRUE(sub_response_ptr != nullptr);
    auto& sub_response = *sub_response_ptr;
    std::string subscriptionId = "unknown-id";//sub_response["subscriptionId"].GetString();

    // Now, unsubscribe
    auto unsub_request = std::make_shared<RequestData>();
    unsub_request->requestId = "test-unsub-01";
    unsub_request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    unsub_request->request_doc->Parse(
        (std::string(R"({
            "action": "unsubscribe",
            "subscriptionId": ")") + subscriptionId + R"(",
            "requestId": "test-unsub-01"
        })").c_str()
    );
    request_queue->push(unsub_request);

    std::shared_ptr<PooledResponseMap> unsub_response_ptr;
    unsub_request->responseChan->wait_and_pop(unsub_response_ptr);
    ASSERT_TRUE(unsub_response_ptr != nullptr);
    auto& unsub_response = *unsub_response_ptr;

    EXPECT_TRUE(unsub_response.HasMember("action"));
    EXPECT_STREQ(unsub_response["action"].GetString(), "unsubscribe");
    EXPECT_TRUE(unsub_response.HasMember("error"));
    const auto& error = unsub_response["error"];
    EXPECT_STREQ(error["number"].GetString(), "404");
    EXPECT_STREQ(error["reason"].GetString(), "unavailable_data");
    EXPECT_STREQ(error["description"].GetString(), "Subscription ID not found.");
    EXPECT_TRUE(unsub_response.HasMember("requestId"));
    EXPECT_STREQ(unsub_response["requestId"].GetString(), "test-unsub-01");
}

TEST_F(ServiceManagerTest, ProcessUnknownRequest) {
    // Prepare a "set" request for an actuator
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-set-01";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "update",
        "path": "Vehicle.Speed",
        "value": "60",
        "requestId": "test-set-01"
    })");

    request_queue->push(request);

    std::shared_ptr<PooledResponseMap> response_map_ptr;
    request->responseChan->wait_and_pop(response_map_ptr);
    ASSERT_TRUE(response_map_ptr != nullptr);
    auto& response_map = *response_map_ptr;

    EXPECT_TRUE(response_map.HasMember("action"));
    EXPECT_STREQ(response_map["action"].GetString(), "update");
    EXPECT_TRUE(response_map.HasMember("requestId"));
    EXPECT_STREQ(response_map["requestId"].GetString(), "test-set-01");
    EXPECT_TRUE(response_map.HasMember("error"));
}

TEST_F(ServiceManagerTest, ProcessGetMetadataWithInvalidDepthString) {
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-meta-invalid-depth";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.Speed",
        "filter": [{"variant": "metadata", "parameter": ["not_a_number"]}],
        "requestId": "test-meta-invalid-depth"
    })");
    request_queue->push(request);

    std::shared_ptr<PooledResponseMap> response_ptr;
    request->responseChan->wait_and_pop(response_ptr);
    ASSERT_TRUE(response_ptr != nullptr);
    auto& response_map = *response_ptr;

    EXPECT_TRUE(response_map.HasMember("action"));
    EXPECT_STREQ(response_map["action"].GetString(), "get");
    EXPECT_TRUE(response_map.HasMember("metadata"));
}

TEST_F(ServiceManagerTest, ProcessSetRequestNotFoundAndInvalidData) {
    // 1) Non-existent path -> 404
    auto req1 = std::make_shared<RequestData>();
    req1->requestId = "test-set-not-found";
    req1->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    req1->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.NonExistent.Path",
        "value": "100",
        "requestId": "test-set-not-found"
    })");
    request_queue->push(req1);

    std::shared_ptr<PooledResponseMap> res1_ptr;
    req1->responseChan->wait_and_pop(res1_ptr);
    ASSERT_TRUE(res1_ptr != nullptr);
    EXPECT_TRUE((*res1_ptr).HasMember("error"));
    EXPECT_STREQ((*res1_ptr)["error"]["number"].GetString(), "404");

    // 2) Sensor node -> 400 Invalid Data (update of sensor not supported)
    auto req2 = std::make_shared<RequestData>();
    req2->requestId = "test-set-sensor";
    req2->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    req2->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.Speed",
        "value": "100",
        "requestId": "test-set-sensor"
    })");
    request_queue->push(req2);

    std::shared_ptr<PooledResponseMap> res2_ptr;
    req2->responseChan->wait_and_pop(res2_ptr);
    ASSERT_TRUE(res2_ptr != nullptr);
    EXPECT_TRUE((*res2_ptr).HasMember("error"));
    EXPECT_STREQ((*res2_ptr)["error"]["number"].GetString(), "400");
}

TEST_F(ServiceManagerTest, ProcessUnsubscribeMissingId) {
    auto request = std::make_shared<RequestData>();
    request->requestId = "test-unsub-missing-id";
    request->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    request->request_doc->Parse(R"({
        "action": "unsubscribe",
        "requestId": "test-unsub-missing-id"
    })");
    request_queue->push(request);

    std::shared_ptr<PooledResponseMap> response_ptr;
    request->responseChan->wait_and_pop(response_ptr);
    ASSERT_TRUE(response_ptr != nullptr);
    EXPECT_TRUE((*response_ptr).HasMember("error"));
    EXPECT_STREQ((*response_ptr)["error"]["number"].GetString(), "400");
}

TEST_F(ServiceManagerTest, ProcessGetMultipleValues) {
    // Set 2 values for ABS
    auto set1 = std::make_shared<RequestData>();
    set1->requestId = "set-1";
    set1->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    set1->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "value": "true",
        "requestId": "set-1"
    })");
    request_queue->push(set1);
    std::shared_ptr<PooledResponseMap> set1_res;
    set1->responseChan->wait_and_pop(set1_res);

    auto set2 = std::make_shared<RequestData>();
    set2->requestId = "set-2";
    set2->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    set2->request_doc->Parse(R"({
        "action": "set",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "value": "false",
        "requestId": "set-2"
    })");
    request_queue->push(set2);
    std::shared_ptr<PooledResponseMap> set2_res;
    set2->responseChan->wait_and_pop(set2_res);

    // Get with history filter
    auto get_req = std::make_shared<RequestData>();
    get_req->requestId = "get-history";
    get_req->responseChan = std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>();
    get_req->request_doc->Parse(R"({
        "action": "get",
        "path": "Vehicle.ADAS.ABS.IsEnabled",
        "filter": [{"variant": "history", "parameter": ["PT1M"]}],
        "requestId": "get-history"
    })");
    request_queue->push(get_req);

    std::shared_ptr<PooledResponseMap> get_res;
    get_req->responseChan->wait_and_pop(get_res);
    ASSERT_TRUE(get_res != nullptr);
    EXPECT_TRUE((*get_res).HasMember("data"));
}
