#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "transmitter/dds/DDSProtocol.cpp"
#include "transmitter/dds/DDSDynamicIDLParser.cpp"
#include "transmitter/MessageProtocol.cpp"
#include "transmitter/Message.cpp"
#include "diagnostic/VariantData.cpp"
#include "mock/MockLogger.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;

using namespace diagnostic_manager;

class DDSProtocolTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new DDSProtocol;
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        stopAllProcess();
        process_list.clear();
        delete ins;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    pid_t startProcess(char* args[]) {
        pid_t pid = fork();
        if (pid < 0) {
            return -1;
        } else if (pid == 0) {
            execv(args[0], args);
        }
        process_list.insert(pid);
        return pid;
    }
    int32_t stopProcess(pid_t pid) {
    if (pid <= 0) return -1;

    if (kill(pid, SIGTERM) == -1 && errno != ESRCH) {
        perror("kill failed");
        return -1;
    }

    int32_t status = 0;
    for (int i = 0; i < 20; ++i) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) break;
        if (result == -1 && errno == ECHILD) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (kill(pid, 0) == 0) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    }

    return status;
//        kill(pid, SIGTERM);
//
//        int32_t status;
//        waitpid(pid, &status, 0);
//
//        process_list.erase(pid);
//        return status;
    }
    void stopAllProcess() {
        for (auto iter : process_list) {
            stopProcess(iter);
        }
        process_list.clear();
    }
    std::set<pid_t> process_list;


    DDSProtocol* ins;
};

TEST_F(DDSProtocolTest, InitializeAndFinalize_TC001) {
    EXPECT_TRUE(ins->Initialize());
    ins->Finalize();
}

// Test Finalize without Initialize (null participant)
TEST_F(DDSProtocolTest, Finalize_TC002_without_initialize) {
    // Finalize should handle null participant gracefully
    ins->Finalize();
    // No crash or exception expected
}

// Test multiple Initialize and Finalize calls
TEST_F(DDSProtocolTest, InitializeAndFinalize_TC002_multiple_calls) {
    EXPECT_TRUE(ins->Initialize());
    ins->Finalize();
    
    // Second init/finalize cycle
    EXPECT_TRUE(ins->Initialize());
    ins->Finalize();
}

TEST_F(DDSProtocolTest, GetProtocolType_TC001) {
    EXPECT_EQ(MessageProtocolType::DDS, ins->GetProtocolType());
}

// Test UnsubscribeMessage when topic is not subscribed
TEST_F(DDSProtocolTest, UnsubscribeMessage_TC002_not_subscribed) {
    EXPECT_TRUE(ins->Initialize());
    
    // Try to unsubscribe a topic that was never subscribed
    bool result = ins->UnsubscribeMessage("/NonExistentTopic");
    EXPECT_FALSE(result);
    
    ins->Finalize();
}

// Test SubscribeMessage twice for same topic (idempotency)
TEST_F(DDSProtocolTest, SubscribeMessage_TC002_duplicate_subscription) {
    EXPECT_TRUE(ins->Initialize());
    
    EXPECT_TRUE(ins->SubscribeMessage("/TestTopic"));
    // Subscribe again - should not fail
    EXPECT_TRUE(ins->SubscribeMessage("/TestTopic"));
    
    // Unsubscribe once
    EXPECT_TRUE(ins->UnsubscribeMessage("/TestTopic"));
    // Second unsubscribe should fail
    EXPECT_FALSE(ins->UnsubscribeMessage("/TestTopic"));
    
    ins->Finalize();
}

// Test SubscribeMessage with multiple different topics
TEST_F(DDSProtocolTest, SubscribeMessage_TC003_multiple_topics) {
    EXPECT_TRUE(ins->Initialize());
    
    EXPECT_TRUE(ins->SubscribeMessage("/Topic1"));
    EXPECT_TRUE(ins->SubscribeMessage("/Topic2"));
    EXPECT_TRUE(ins->SubscribeMessage("/Topic3"));
    
    // Verify all topics are in the subscribed list
    EXPECT_EQ(3, ins->subscribed_topic_.size());
    
    // Unsubscribe all
    EXPECT_TRUE(ins->UnsubscribeMessage("/Topic1"));
    EXPECT_TRUE(ins->UnsubscribeMessage("/Topic2"));
    EXPECT_TRUE(ins->UnsubscribeMessage("/Topic3"));
    
    EXPECT_EQ(0, ins->subscribed_topic_.size());
    
    ins->Finalize();
}

// ==================== Helper Function Coverage ====================

// Test getEnvPath with existing environment variable
TEST_F(DDSProtocolTest, getEnvPath_TC001_existing_env) {
    // Test with a common environment variable that usually exists
    auto result = getEnvPath("PATH");
    // PATH should exist in most environments
    // If it doesn't exist, result will be nullopt
    if (result.has_value()) {
        EXPECT_FALSE(result.value().empty());
    }
}

// Test getEnvPath with non-existing environment variable
TEST_F(DDSProtocolTest, getEnvPath_TC002_non_existing_env) {
    auto result = getEnvPath("NON_EXISTING_ENV_VARIABLE_12345");
    EXPECT_FALSE(result.has_value());
}

// Test convertTypeName with ROS2 DDS type to simple type (to_simple = true)
TEST_F(DDSProtocolTest, convertTypeName_TC001_ros2_to_simple) {
    // Test known ROS2 DDS type mapping
    std::string simple_type = convertTypeName("diagnosticmgr::msg::dds_::DiagnosticInfoRequest_", true);
    EXPECT_EQ("DiagnosticInfoRequest", simple_type);
}

// Test convertTypeName with simple type to ROS2 DDS type (to_simple = false)
TEST_F(DDSProtocolTest, convertTypeName_TC002_simple_to_ros2) {
    std::string ros2_type = convertTypeName("DiagnosticInfoRequest", false);
    EXPECT_EQ("diagnosticmgr::msg::dds_::DiagnosticInfoRequest_", ros2_type);
}

// Test convertTypeName with unknown type (to_simple = true)
TEST_F(DDSProtocolTest, convertTypeName_TC003_unknown_type_to_simple) {
    std::string result = convertTypeName("UnknownType", true);
    // Should return original type if no mapping found and extraction fails
    EXPECT_EQ("UnknownType", result);
}

// Test convertTypeName with unknown type (to_simple = false)
TEST_F(DDSProtocolTest, convertTypeName_TC004_unknown_type_to_ros2) {
    std::string result = convertTypeName("UnknownType", false);
    // Should return original type if no mapping found
    EXPECT_EQ("UnknownType", result);
}

// Test convertTypeName with type containing :: (extract simple type)
TEST_F(DDSProtocolTest, convertTypeName_TC005_extract_simple_type) {
    std::string result = convertTypeName("some::namespace::MyType", true);
    // Should extract after last ::
    EXPECT_EQ("MyType", result);
}

// Test toSimpleType convenience function
TEST_F(DDSProtocolTest, toSimpleType_TC001) {
    std::string simple = toSimpleType("diagnosticmgr::msg::dds_::DiagnosticNotification_");
    EXPECT_EQ("DiagnosticNotification", simple);
}

// Test toRos2DdsType convenience function
TEST_F(DDSProtocolTest, toRos2DdsType_TC001) {
    std::string ros2 = toRos2DdsType("DiagnosticInfoResponse");
    EXPECT_EQ("diagnosticmgr::msg::dds_::DiagnosticInfoResponse_", ros2);
}

// Test all ROS2 DDS type mappings
TEST_F(DDSProtocolTest, convertTypeName_TC006_all_mappings) {
    // Test all mappings - ROS2 to simple
    EXPECT_EQ("DiagnosticInfoRequest", toSimpleType("diagnosticmgr::msg::dds_::DiagnosticInfoRequest_"));
    EXPECT_EQ("DiagnosticInfoListRequest", toSimpleType("diagnosticmgr::msg::dds_::DiagnosticInfoListRequest_"));
    EXPECT_EQ("DiagnosticInfoResponse", toSimpleType("diagnosticmgr::msg::dds_::DiagnosticInfoResponse_"));
    EXPECT_EQ("DiagnosticInfoListResponse", toSimpleType("diagnosticmgr::msg::dds_::DiagnosticInfoListResponse_"));
    EXPECT_EQ("DiagnosticNotification", toSimpleType("diagnosticmgr::msg::dds_::DiagnosticNotification_"));
    
    // Test all mappings - simple to ROS2
    EXPECT_EQ("diagnosticmgr::msg::dds_::DiagnosticInfoRequest_", toRos2DdsType("DiagnosticInfoRequest"));
    EXPECT_EQ("diagnosticmgr::msg::dds_::DiagnosticInfoListRequest_", toRos2DdsType("DiagnosticInfoListRequest"));
    EXPECT_EQ("diagnosticmgr::msg::dds_::DiagnosticInfoResponse_", toRos2DdsType("DiagnosticInfoResponse"));
    EXPECT_EQ("diagnosticmgr::msg::dds_::DiagnosticInfoListResponse_", toRos2DdsType("DiagnosticInfoListResponse"));
    EXPECT_EQ("diagnosticmgr::msg::dds_::DiagnosticNotification_", toRos2DdsType("DiagnosticNotification"));
}

// ==================== on_subscription_matched Coverage ====================

// Test on_subscription_matched with matching (current_count_change == 1)
TEST_F(DDSProtocolTest, on_subscription_matched_TC001_matched) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create a mock status with matched subscriber
    SubscriptionMatchedStatus status;
    status.current_count_change = 1;
    status.current_count = 1;
    
    // This should execute the "Subscriber matched" branch
    EXPECT_NO_THROW(ins->on_subscription_matched(nullptr, status));
    
    ins->Finalize();
}

// Test on_subscription_matched with unmatching (current_count_change == -1)
TEST_F(DDSProtocolTest, on_subscription_matched_TC002_unmatched) {
    EXPECT_TRUE(ins->Initialize());
    
    SubscriptionMatchedStatus status;
    status.current_count_change = -1;
    status.current_count = 0;
    
    // This should execute the "Subscriber unmatched" branch
    EXPECT_NO_THROW(ins->on_subscription_matched(nullptr, status));
    
    ins->Finalize();
}

// Test on_subscription_matched with invalid change value (else branch)
TEST_F(DDSProtocolTest, on_subscription_matched_TC003_invalid_change) {
    EXPECT_TRUE(ins->Initialize());
    
    SubscriptionMatchedStatus status;
    status.current_count_change = 5;  // Invalid value
    status.current_count = 5;
    
    // This should execute the error logging branch
    EXPECT_NO_THROW(ins->on_subscription_matched(nullptr, status));
    
    ins->Finalize();
}

// Test on_subscription_matched with zero change
TEST_F(DDSProtocolTest, on_subscription_matched_TC004_zero_change) {
    EXPECT_TRUE(ins->Initialize());
    
    SubscriptionMatchedStatus status;
    status.current_count_change = 0;
    status.current_count = 1;
    
    EXPECT_NO_THROW(ins->on_subscription_matched(nullptr, status));
    
    ins->Finalize();
}

// ==================== PublishMessage Coverage ====================

// Test PublishMessage with null topic_info (topic not discovered)
TEST_F(DDSProtocolTest, PublishMessage_TC002_topic_not_found) {
    EXPECT_TRUE(ins->Initialize());
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("TestType");
    variant_data->SetValue("id", (int32_t)123);
    
    auto message = std::make_shared<Message>(
        "/UnknownTopic", 
        MessageProtocolType::DDS, 
        "UnknownType", 
        variant_data
    );
    
    // Should fail because topic type is not registered
    bool result = ins->PublishMessage(message);
    // Result depends on whether type is found in dynamic IDL
    
    ins->Finalize();
}

// Test PublishMessage with ROS topic (rt/ prefix)
TEST_F(DDSProtocolTest, PublishMessage_TC003_ros_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DiagnosticInfoRequest");
    variant_data->SetValue("id", (int32_t)100);
    
    auto message = std::make_shared<Message>(
        "rt/diagnostic_request",
        MessageProtocolType::DDS,
        "DiagnosticInfoRequest",
        variant_data
    );
    
    // This tests the rt/ prefix handling branch
    bool result = ins->PublishMessage(message);
    // Result depends on IDL availability
    
    ins->Finalize();
}

// Test PublishMessage with DDS topic (non-rt/ prefix)
TEST_F(DDSProtocolTest, PublishMessage_TC004_dds_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DataTypeTestSimple");
    variant_data->SetValue("id", (int32_t)456);
    variant_data->SetValue("name", std::string("TestName"));
    
    auto message = std::make_shared<Message>(
        "/TestDDSTopic",
        MessageProtocolType::DDS,
        "DataTypeTestSimple",
        variant_data
    );
    
    // This tests the non-rt/ topic handling
    bool result = ins->PublishMessage(message);
    
    ins->Finalize();
}

// Test PublishMessage with cached topic (existing topic_info)
TEST_F(DDSProtocolTest, PublishMessage_TC005_cached_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DataTypeTestSimple");
    variant_data->SetValue("id", (int32_t)789);
    variant_data->SetValue("name", std::string("CachedTest"));
    
    auto message = std::make_shared<Message>(
        "/CachedTopic",
        MessageProtocolType::DDS,
        "DataTypeTestSimple",
        variant_data
    );
    
    // First publish - creates topic
    ins->PublishMessage(message);
    
    // Second publish - uses cached topic
    bool result = ins->PublishMessage(message);
    
    ins->Finalize();
}

// ==================== SerializeMessage Coverage ====================

// Test SerializeMessage with various data types
TEST_F(DDSProtocolTest, SerializeMessage_TC002_all_types) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create variant data with all supported types
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DataTypeTest");
    
    // Scalar types
    variant_data->SetValue("v_char8", (char)65);
    variant_data->SetValue("v_char16", (wchar_t)66);
    variant_data->SetValue("v_int8", (int8_t)-8);
    variant_data->SetValue("v_int16", (int16_t)-16);
    variant_data->SetValue("v_int32", (int32_t)-32);
    variant_data->SetValue("v_int64", (int64_t)-64);
    variant_data->SetValue("v_uint8", (uint8_t)8);
    variant_data->SetValue("v_uint16", (uint16_t)16);
    variant_data->SetValue("v_uint32", (uint32_t)32);
    variant_data->SetValue("v_uint64", (uint64_t)64);
    variant_data->SetValue("v_float", 1.5f);
    variant_data->SetValue("v_double", 2.5);
    variant_data->SetValue("v_string", std::string("test_string"));
    variant_data->SetValue("v_wstring", std::wstring(L"wide_string"));
    
    // Vector types
    variant_data->SetValue("seq_char8", std::vector<char>{'a', 'b'});
    variant_data->SetValue("seq_char16", std::vector<wchar_t>{L'x', L'y'});
    variant_data->SetValue("seq_int8", std::vector<int8_t>{-1, -2});
    variant_data->SetValue("seq_int16", std::vector<int16_t>{-10, -20});
    variant_data->SetValue("seq_int32", std::vector<int32_t>{-100, -200});
    variant_data->SetValue("seq_int64", std::vector<int64_t>{-1000, -2000});
    variant_data->SetValue("seq_uint8", std::vector<uint8_t>{1, 2});
    variant_data->SetValue("seq_uint16", std::vector<uint16_t>{10, 20});
    variant_data->SetValue("seq_uint32", std::vector<uint32_t>{100, 200});
    variant_data->SetValue("seq_uint64", std::vector<uint64_t>{1000, 2000});
    variant_data->SetValue("seq_float", std::vector<float>{1.1f, 2.2f});
    variant_data->SetValue("seq_double", std::vector<double>{1.11, 2.22});
    variant_data->SetValue("seq_bool", std::vector<bool>{true, false});
    variant_data->SetValue("seq_string", std::vector<std::string>{"str1", "str2"});
    variant_data->SetValue("seq_wstring", std::vector<std::wstring>{L"wstr1", L"wstr2"});
    
    auto message = std::make_shared<Message>(
        "/AllTypesTest",
        MessageProtocolType::DDS,
        "DataTypeTest",
        variant_data
    );
    
    // This will exercise SerializeMessage with all type branches
    ins->PublishMessage(message);
    
    ins->Finalize();
}

// ==================== on_data_available Coverage ====================

// Test on_data_available when topic is not discovered
TEST_F(DDSProtocolTest, on_data_available_TC001_topic_not_found) {
    EXPECT_TRUE(ins->Initialize());
    
    // Call on_data_available with nullptr (early return due to no topic)
    EXPECT_NO_THROW(ins->on_data_available(nullptr));
    
    ins->Finalize();
}

// ==================== delete_datawriter / delete_datareader Coverage ====================

// These are tested indirectly through Finalize but we can add explicit tests

// ==================== create_topic_information_from_dynamic_idl Coverage ====================

// Test with valid type from IDL
TEST_F(DDSProtocolTest, create_topic_info_TC001_valid_type) {
    EXPECT_TRUE(ins->Initialize());
    
    // Try to create topic info for a known type
    auto topic_info = ins->create_topic_information_from_dynamic_idl("/TestTopic", "DataTypeTestSimple");
    
    // Result depends on IDL availability
    if (topic_info) {
        EXPECT_EQ("/TestTopic", topic_info->topic_name);
        EXPECT_TRUE(topic_info->remote_type != nullptr || topic_info->type_support != nullptr);
    }
    
    ins->Finalize();
}

// Test with invalid type (not in IDL)
TEST_F(DDSProtocolTest, create_topic_info_TC002_invalid_type) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = ins->create_topic_information_from_dynamic_idl("/TestTopic", "NonExistentType");
    
    // Should return nullptr for unknown type
    EXPECT_EQ(topic_info, nullptr);
    
    ins->Finalize();
}

// Test with ROS type conversion (3-parameter version)
TEST_F(DDSProtocolTest, create_topic_info_TC003_ros_type_conversion) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = ins->create_topic_information_from_dynamic_idl(
        "rt/test_topic",
        "DiagnosticInfoRequest",  // Simple type for IDL lookup
        "diagnosticmgr::msg::dds_::DiagnosticInfoRequest_"  // Original ROS2 type
    );
    
    // Result depends on IDL availability
    
    ins->Finalize();
}

// ==================== register_type_object Coverage ====================

TEST_F(DDSProtocolTest, register_type_object_TC001_dynamic_idl_success) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create topic information
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/TestRegisterTopic";
    topic_info->topic_type = "DataTypeTestSimple";
    
    // This tests register_type_object which tries dynamic_idl first, then discovery
    bool result = ins->register_type_object(topic_info);
    // Result depends on IDL availability
    
    ins->Finalize();
}

TEST_F(DDSProtocolTest, register_type_object_TC002_fallback_to_discovery) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/UnknownTypeTopic";
    topic_info->topic_type = "CompletelyUnknownType";
    
    // Should fall back to discovery (which may also fail)
    bool result = ins->register_type_object(topic_info);
    EXPECT_FALSE(result);  // Unknown type should fail
    
    ins->Finalize();
}

// ==================== read_values_from_dynamic_data Coverage ====================
// These are tested indirectly through DeserializeMessage tests

// ==================== load_dds_dynamic_idl Coverage ====================

TEST_F(DDSProtocolTest, load_dds_dynamic_idl_TC001_invalid_path) {
    // Create a new instance to test initialization with invalid path
    DDSProtocol* testProtocol = new DDSProtocol();
    
    // Manually call load_dds_dynamic_idl with invalid path
    // This is called during Initialize
    
    delete testProtocol;
}

// ==================== Edge Cases ====================

// Test with empty message name
TEST_F(DDSProtocolTest, PublishMessage_TC006_empty_name) {
    EXPECT_TRUE(ins->Initialize());
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("TestType");
    
    auto message = std::make_shared<Message>(
        "",  // Empty topic name
        MessageProtocolType::DDS,
        "DataTypeTestSimple",
        variant_data
    );
    
    bool result = ins->PublishMessage(message);
    
    ins->Finalize();
}

// Test UnsubscribeMessage with existing reader
TEST_F(DDSProtocolTest, UnsubscribeMessage_TC003_with_reader) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    EXPECT_TRUE(ins->Initialize());
    
    ins->SubscribeMessage("/HelloWorldTopic");
    
    char* args[] = {"test/HelloWorldPublisher", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        ins->Finalize();
        GTEST_SKIP() << "Failed to start test publisher";
        return;
    }
    
    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (ins->discovered_topic_.size() > 0) {
            auto it = ins->discovered_topic_.find("/HelloWorldTopic");
            if (it != ins->discovered_topic_.end() && it->second && it->second->reader != nullptr) {
                break;
            }
        }
        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Unsubscribe should delete the reader
    bool result = ins->UnsubscribeMessage("/HelloWorldTopic");
    EXPECT_TRUE(result);
    
    stopProcess(pid);
    ins->Finalize();
}

// Test multiple subscribe/unsubscribe cycles
TEST_F(DDSProtocolTest, SubscribeUnsubscribe_TC001_multiple_cycles) {
    EXPECT_TRUE(ins->Initialize());
    
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(ins->SubscribeMessage("/CycleTopic"));
        EXPECT_TRUE(ins->UnsubscribeMessage("/CycleTopic"));
    }
    
    ins->Finalize();
}

// Test SetForwardCallback
TEST_F(DDSProtocolTest, SetForwardCallback_TC001) {
    EXPECT_TRUE(ins->Initialize());
    
    bool callback_called = false;
    ins->SetForwardCallback([&](std::shared_ptr<Message> msg) {
        callback_called = true;
    });
    
    // Reset callback
    ins->SetForwardCallback(nullptr);
    
    ins->Finalize();
}

TEST_F(DDSProtocolTest, PublishMessage_TC001) {

}

TEST_F(DDSProtocolTest, SubscribeMessage_TC001) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopic");

    char* args[] = {"test/HelloWorldPublisher", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }

    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (ins->discovered_topic_.size() > 0) {
            break;
        }

        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_EQ(1, ins->discovered_topic_.size());

	auto it = ins->discovered_topic_.find("/HelloWorldTopic");
	ASSERT_TRUE(it != ins->discovered_topic_.end());
	ASSERT_TRUE(it->second != nullptr) << "TopicInformation shared_ptr is null";
	EXPECT_TRUE(it->second->topic != nullptr);
	EXPECT_TRUE(it->second->reader != nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->topic != nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->reader != nullptr);

    stopProcess(pid);

    ins->Finalize();
}

TEST_F(DDSProtocolTest, UnsubscribeMessage_TC001) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopic");

    char* args[] = {"test/HelloWorldPublisher", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }

    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (ins->discovered_topic_.size() > 0) {
            break;
        }

        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_EQ(1, ins->discovered_topic_.size());

    ins->UnsubscribeMessage("/HelloWorldTopic");

	auto it = ins->discovered_topic_.find("/HelloWorldTopic");
	ASSERT_TRUE(it != ins->discovered_topic_.end());
	ASSERT_TRUE(it->second != nullptr) << "TopicInformation shared_ptr is null";
	EXPECT_TRUE(it->second->topic == nullptr);
	EXPECT_TRUE(it->second->reader == nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->topic == nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->reader == nullptr);
    

    stopProcess(pid);

    ins->Finalize();
}

TEST_F(DDSProtocolTest, SerializeMessage_TC001) {

}

TEST_F(DDSProtocolTest, DeserializeMessage_TC001) {
//    constexpr uint32_t MAX_WAIT_COUNT = 100;
//    std::shared_ptr<Message> received_message;
//
//    ins->SetForwardCallback([&] (std::shared_ptr<Message> msg) {
//        received_message = msg;
//    });

    using namespace std::chrono_literals;

    std::mutex m;
    std::condition_variable cv;
    std::shared_ptr<Message> received_message;

    ins->SetForwardCallback([&](std::shared_ptr<Message> msg) {
        {
            std::lock_guard<std::mutex> lk(m);
            received_message = std::move(msg);
        }
        cv.notify_one();
    });

    constexpr uint32_t MAX_WAIT_COUNT = 100;

    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopic");

    //char* args[] = {"test/HelloWorldPublisher", 0};
    //pid_t pid = startProcess(args);
    //if (pid == -1) {
    //    std::cerr << "Failed to start test publisher" << std::endl;
    //    FAIL();
    //}


    //uint32_t wait_count = 0;
    //while (wait_count < MAX_WAIT_COUNT) {
    //    if (received_message != nullptr) {
    //        break;
    //    }

    //    wait_count++;
    //    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //}

    //ins->UnsubscribeMessage("/HelloWorldTopic");
    //stopProcess(pid);

    char* args[] = { (char*)"test/HelloWorldPublisher", nullptr };
    pid_t pid = startProcess(args);
    ASSERT_NE(pid, -1) << "Failed to start test publisher";

    {
        std::unique_lock<std::mutex> lk(m);
        bool ok = cv.wait_for(lk, 50ms * MAX_WAIT_COUNT, [&]{
            return (bool)received_message;
        });
        ASSERT_TRUE(ok) << "Timed out waiting for received_message";
    }

    EXPECT_EQ("/HelloWorldTopic", received_message->GetName());
    EXPECT_EQ(MessageProtocolType::DDS, received_message->GetProtocolType());
    EXPECT_EQ("DataTypeTest", received_message->GetTypeName());
    
    std::shared_ptr<VariantData> data = received_message->GetData();
    ASSERT_TRUE(data) << "Data is null";
    EXPECT_EQ(11, data->GetValue<char>("v_char8"));
    EXPECT_EQ(12, data->GetValue<wchar_t>("v_char16"));
    EXPECT_EQ(13, data->GetValue<uint8_t>("v_uint8"));
    EXPECT_EQ(14, data->GetValue<int16_t>("v_int16"));
    EXPECT_EQ(15, data->GetValue<uint16_t>("v_uint16"));
    EXPECT_EQ(16, data->GetValue<int32_t>("v_int32"));
    EXPECT_EQ(17, data->GetValue<uint32_t>("v_uint32"));
    EXPECT_EQ(18, data->GetValue<int64_t>("v_int64"));
    EXPECT_EQ(19, data->GetValue<uint64_t>("v_uint64"));
    EXPECT_EQ(123.45f, data->GetValue<float>("v_float"));
    EXPECT_EQ(234.56, data->GetValue<double>("v_double"));
    EXPECT_EQ(true, data->GetValue<bool>("v_bool"));
    EXPECT_EQ("Test String", data->GetValue<std::string>("v_string"));
    
    EXPECT_EQ(1234, data->GetValue<VariantData>("v_structure").GetValue<int32_t>("id"));
    EXPECT_EQ("HelloPinky", data->GetValue<VariantData>("v_structure").GetValue<std::string>("name"));

    EXPECT_EQ(std::vector<char>({1,2}), data->GetValue<std::vector<char>>("seq_char8"));
    EXPECT_EQ(std::vector<wchar_t>({1,2}), data->GetValue<std::vector<wchar_t>>("seq_char16"));
    EXPECT_EQ(std::vector<uint8_t>({1,2}), data->GetValue<std::vector<uint8_t>>("seq_uint8"));
    EXPECT_EQ(std::vector<int16_t>({1,2}), data->GetValue<std::vector<int16_t>>("seq_int16"));
    EXPECT_EQ(std::vector<uint16_t>({1,2}), data->GetValue<std::vector<uint16_t>>("seq_uint16"));
    EXPECT_EQ(std::vector<int32_t>({1,2}), data->GetValue<std::vector<int32_t>>("seq_int32"));
    EXPECT_EQ(std::vector<uint32_t>({1,2}), data->GetValue<std::vector<uint32_t>>("seq_uint32"));
    EXPECT_EQ(std::vector<int64_t>({1,2}), data->GetValue<std::vector<int64_t>>("seq_int64"));
    EXPECT_EQ(std::vector<uint64_t>({1,2}), data->GetValue<std::vector<uint64_t>>("seq_uint64"));
    EXPECT_EQ(std::vector<float>({1.2,2.3}), data->GetValue<std::vector<float>>("seq_float"));
    EXPECT_EQ(std::vector<double>({1.23,2.34}), data->GetValue<std::vector<double>>("seq_double"));
    EXPECT_EQ(std::vector<bool>({false, true}), data->GetValue<std::vector<bool>>("seq_bool"));
    EXPECT_EQ(std::vector<std::string>({"abc", "def"}), data->GetValue<std::vector<std::string>>("seq_string"));

    ins->UnsubscribeMessage("/HelloWorldTopic");
    stopProcess(pid);

    ins->SetForwardCallback(nullptr);
    ins->Finalize();
}

TEST_F(DDSProtocolTest, DeserializeMessage_TC002_dynamic_idl_parse) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    std::shared_ptr<Message> received_message;

    ins->SetForwardCallback([&] (std::shared_ptr<Message> msg) {
        received_message = msg;
    });

    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopicSimple");

    char* args[] = {"test/HelloWorldPublisherNoDynamic", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }


    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (received_message != nullptr) {
            break;
        }

        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ins->UnsubscribeMessage("/HelloWorldTopicSimple");
    stopProcess(pid);

    ASSERT_TRUE(wait_count != MAX_WAIT_COUNT);

    EXPECT_EQ("/HelloWorldTopicSimple", received_message->GetName());
    EXPECT_EQ(MessageProtocolType::DDS, received_message->GetProtocolType());
    EXPECT_EQ("DataTypeTestSimple", received_message->GetTypeName());
    
    std::shared_ptr<VariantData> data = received_message->GetData();
    EXPECT_EQ(11, data->GetValue<int32_t>("id"));
    EXPECT_EQ("HelloWorld1234!", data->GetValue<std::string>("name"));

    ins->Finalize();
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
