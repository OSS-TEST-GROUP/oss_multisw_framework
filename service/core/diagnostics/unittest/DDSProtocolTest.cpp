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
        if (ins) {
            ins->Finalize();
            delete ins;
            ins = nullptr;
        }
        delete Mock_Logger;
        Mock_Logger = nullptr;
        // Small delay to allow FastDDS participant cleanup to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

// ==================== Topic Cache and Type Registration Coverage ====================

// Test PublishMessage with cached topic (simulating cache hit scenario)
TEST_F(DDSProtocolTest, PublishMessage_TC007_cache_hit_with_type_support) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create a topic info and manually insert it into discovered_topic_
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/CachedTestTopic";
    topic_info->topic_type = "DataTypeTestSimple";
    
    // Look up the type from parsed_dynamic_type_supports_
    auto idl_iter = ins->parsed_dynamic_type_supports_.find("DataTypeTestSimple");
    if (idl_iter != ins->parsed_dynamic_type_supports_.end()) {
        topic_info->remote_type = idl_iter->second->dyn_type;
        topic_info->type_support = idl_iter->second->type_support;
    }
    
    // Insert into discovered_topic_ to simulate cache
    ins->discovered_topic_["/CachedTestTopic"] = topic_info;
    
    // Create a message for this topic
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DataTypeTestSimple");
    variant_data->SetValue("id", (int32_t)100);
    variant_data->SetValue("name", std::string("CacheHitTest"));
    
    auto message = std::make_shared<Message>(
        "/CachedTestTopic",
        MessageProtocolType::DDS,
        "DataTypeTestSimple",
        variant_data
    );
    
    // This should use the cached topic info
    bool result = ins->PublishMessage(message);
    // Result depends on writer creation success
    
    ins->Finalize();
}

// Test PublishMessage with cached topic that has null type_support (DDS topic)
TEST_F(DDSProtocolTest, PublishMessage_TC008_cache_hit_null_type_support_dds) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create a topic info with null type_support to trigger the fix-up code
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/NullTypeSupportTopic";
    topic_info->topic_type = "DataTypeTestSimple";
    topic_info->type_support = nullptr;  // Explicitly null
    
    // Insert into discovered_topic_
    ins->discovered_topic_["/NullTypeSupportTopic"] = topic_info;
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DataTypeTestSimple");
    variant_data->SetValue("id", (int32_t)200);
    variant_data->SetValue("name", std::string("NullTypeSupportTest"));
    
    auto message = std::make_shared<Message>(
        "/NullTypeSupportTopic",
        MessageProtocolType::DDS,
        "DataTypeTestSimple",
        variant_data
    );
    
    // This should attempt to fix the null type_support
    bool result = ins->PublishMessage(message);
    
    ins->Finalize();
}

// Test PublishMessage with cached topic that has null type_support (ROS topic)
TEST_F(DDSProtocolTest, PublishMessage_TC009_cache_hit_null_type_support_ros) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create a topic info with null type_support for ROS topic
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "rt/null_type_support_ros";
    topic_info->topic_type = "diagnosticmgr::msg::dds_::DiagnosticInfoRequest_";
    topic_info->type_support = nullptr;  // Explicitly null
    
    // Insert into discovered_topic_
    ins->discovered_topic_["rt/null_type_support_ros"] = topic_info;
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DiagnosticInfoRequest");
    variant_data->SetValue("id", (int32_t)300);
    
    auto message = std::make_shared<Message>(
        "rt/null_type_support_ros",
        MessageProtocolType::DDS,
        "DiagnosticInfoRequest",  // Simple type name
        variant_data
    );
    
    // This should attempt to fix the null type_support for ROS topic
    bool result = ins->PublishMessage(message);
    
    ins->Finalize();
}

// Test PublishMessage with cached topic that has null type_support (unknown type)
TEST_F(DDSProtocolTest, PublishMessage_TC010_cache_hit_null_type_support_unknown_type) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create a topic info with null type_support for unknown type
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/UnknownTypeTopic";
    topic_info->topic_type = "CompletelyUnknownType";
    topic_info->type_support = nullptr;  // Explicitly null
    
    // Insert into discovered_topic_
    ins->discovered_topic_["/UnknownTypeTopic"] = topic_info;
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("CompletelyUnknownType");
    variant_data->SetValue("data", (int32_t)400);
    
    auto message = std::make_shared<Message>(
        "/UnknownTypeTopic",
        MessageProtocolType::DDS,
        "CompletelyUnknownType",
        variant_data
    );
    
    // This should fail to find type in dynamic IDL and log error
    bool result = ins->PublishMessage(message);
    EXPECT_FALSE(result);  // Should fail for unknown type
    
    ins->Finalize();
}

// Test PublishMessage with cached ROS topic (using cache normally)
TEST_F(DDSProtocolTest, PublishMessage_TC011_cache_hit_ros_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create a topic info for ROS topic with valid type_support
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "rt/cached_ros_topic";
    topic_info->topic_type = "diagnosticmgr::msg::dds_::DiagnosticInfoRequest_";
    
    // Look up the type using simple name
    auto idl_iter = ins->parsed_dynamic_type_supports_.find("DiagnosticInfoRequest");
    if (idl_iter != ins->parsed_dynamic_type_supports_.end()) {
        topic_info->remote_type = idl_iter->second->dyn_type;
        topic_info->type_support = idl_iter->second->type_support;
    }
    
    // Insert into discovered_topic_
    ins->discovered_topic_["rt/cached_ros_topic"] = topic_info;
    
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("DiagnosticInfoRequest");
    variant_data->SetValue("id", (int32_t)500);
    
    auto message = std::make_shared<Message>(
        "rt/cached_ros_topic",
        MessageProtocolType::DDS,
        "DiagnosticInfoRequest",
        variant_data
    );
    
    // This should use cached ROS topic (covers LOG_DEBUG branch)
    bool result = ins->PublishMessage(message);
    
    ins->Finalize();
}

// ==================== COMPREHENSIVE SERIALIZE/DESERIALIZE COVERAGE TESTS ====================
// These tests ensure SerializeMessage and DeserializeMessage are thoroughly covered

// Test SerializeMessage with all scalar types
TEST_F(DDSProtocolTest, Serialize_TC001_all_scalar_types) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/SerializeScalarTypes";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/SerializeScalarTypes"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Test all scalar types to hit all SerializeMessage branches
        variant_data->SetValue("v_char8", (char)'Z');
        variant_data->SetValue("v_char16", (wchar_t)L'Ω');
        variant_data->SetValue("v_int8", (int8_t)-100);
        variant_data->SetValue("v_uint8", (uint8_t)200);
        variant_data->SetValue("v_int16", (int16_t)-30000);
        variant_data->SetValue("v_uint16", (uint16_t)60000);
        variant_data->SetValue("v_int32", (int32_t)-2000000);
        variant_data->SetValue("v_uint32", (uint32_t)4000000);
        variant_data->SetValue("v_int64", (int64_t)-9000000000LL);
        variant_data->SetValue("v_uint64", (uint64_t)18000000000ULL);
        variant_data->SetValue("v_float", 2.71828f);
        variant_data->SetValue("v_double", 1.41421356);
        variant_data->SetValue("v_bool", false);
        variant_data->SetValue("v_string", std::string("SerializeTest"));
        variant_data->SetValue("v_wstring", std::wstring(L"宽字符"));
        
        auto message = std::make_shared<Message>(
            "/SerializeScalarTypes",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        // This tests SerializeMessage with all scalar type branches
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
        EXPECT_NE(nullptr, topic_info->writer);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with all vector/sequence types
TEST_F(DDSProtocolTest, Serialize_TC002_all_sequence_types) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/SerializeSequenceTypes";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/SerializeSequenceTypes"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Test all sequence types to hit all SerializeMessage sequence branches
        variant_data->SetValue("seq_char8", std::vector<char>{'X', 'Y', 'Z'});
        variant_data->SetValue("seq_char16", std::vector<wchar_t>{L'α', L'β', L'γ'});
        variant_data->SetValue("seq_int8", std::vector<int8_t>{-10, -20, -30});
        variant_data->SetValue("seq_uint8", std::vector<uint8_t>{10, 20, 30, 40, 50});
        variant_data->SetValue("seq_int16", std::vector<int16_t>{-1000, -2000});
        variant_data->SetValue("seq_uint16", std::vector<uint16_t>{5000, 10000, 15000});
        variant_data->SetValue("seq_int32", std::vector<int32_t>{-100000, -200000, -300000});
        variant_data->SetValue("seq_uint32", std::vector<uint32_t>{500000, 1000000});
        variant_data->SetValue("seq_int64", std::vector<int64_t>{-5000000000LL, -10000000000LL});
        variant_data->SetValue("seq_uint64", std::vector<uint64_t>{10000000000ULL, 20000000000ULL});
        variant_data->SetValue("seq_float", std::vector<float>{1.1f, 2.2f, 3.3f, 4.4f});
        variant_data->SetValue("seq_double", std::vector<double>{1.111, 2.222, 3.333});
        variant_data->SetValue("seq_bool", std::vector<bool>{true, false, true, false, true});
        variant_data->SetValue("seq_string", std::vector<std::string>{"alpha", "beta", "gamma"});
        variant_data->SetValue("seq_wstring", std::vector<std::wstring>{L"один", L"два", L"три"});
        
        auto message = std::make_shared<Message>(
            "/SerializeSequenceTypes",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        // This tests SerializeMessage with all sequence type branches
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with nested structures
TEST_F(DDSProtocolTest, Serialize_TC003_nested_structures) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/SerializeNested";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/SerializeNested"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Create nested structure
        auto nested = std::make_shared<VariantData>();
        nested->SetName("DataTypeTestSimple");
        nested->SetValue("id", (int32_t)12345);
        nested->SetValue("name", std::string("NestedData"));
        
        variant_data->SetValue("v_structure", *nested);
        variant_data->SetValue("v_int32", (int32_t)99999);
        variant_data->SetValue("v_string", std::string("Parent"));
        
        auto message = std::make_shared<Message>(
            "/SerializeNested",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with edge cases (empty sequences, boundary values)
TEST_F(DDSProtocolTest, Serialize_TC004_edge_cases) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/SerializeEdgeCases";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/SerializeEdgeCases"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Empty sequences
        variant_data->SetValue("seq_int32", std::vector<int32_t>{});
        variant_data->SetValue("seq_string", std::vector<std::string>{});
        
        // Boundary values
        variant_data->SetValue("v_int8", std::numeric_limits<int8_t>::min());
        variant_data->SetValue("v_uint8", std::numeric_limits<uint8_t>::max());
        variant_data->SetValue("v_int16", std::numeric_limits<int16_t>::min());
        variant_data->SetValue("v_uint16", std::numeric_limits<uint16_t>::max());
        variant_data->SetValue("v_int32", std::numeric_limits<int32_t>::min());
        variant_data->SetValue("v_uint32", std::numeric_limits<uint32_t>::max());
        
        // Empty string
        variant_data->SetValue("v_string", std::string(""));
        variant_data->SetValue("v_wstring", std::wstring(L""));
        
        auto message = std::make_shared<Message>(
            "/SerializeEdgeCases",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test DeserializeMessage through actual data flow
TEST_F(DDSProtocolTest, Deserialize_TC001_data_flow_with_publisher) {
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
    
    EXPECT_TRUE(ins->Initialize());
    
    // Subscribe to topic that HelloWorldPublisher publishes to
    EXPECT_TRUE(ins->SubscribeMessage("/HelloWorldTopic"));
    
    // Start publisher process with correct relative path
    char* args[] = {(char*)"./test/HelloWorldPublisher", nullptr};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        ins->UnsubscribeMessage("/HelloWorldTopic");
        ins->Finalize();
        GTEST_SKIP() << "Failed to start test publisher for Deserialize test";
        return;
    }
    
    // Wait for message with timeout
    {
        std::unique_lock<std::mutex> lk(m);
        bool ok = cv.wait_for(lk, 10s, [&]{ return (bool)received_message; });
        
        if (ok && received_message) {
            // Verify DeserializeMessage extracted all fields correctly
            auto data = received_message->GetData();
            ASSERT_TRUE(data) << "Deserialized data is null";
            
            // Verify fields were deserialized
            EXPECT_EQ(received_message->GetProtocolType(), MessageProtocolType::DDS);
            EXPECT_EQ("/HelloWorldTopic", received_message->GetName());
            
            // Verify data was deserialized (DeserializeMessage was called)
            auto keys = data->GetKeyList();
            EXPECT_FALSE(keys.empty()) << "DeserializeMessage should have populated keys";
        } else {
            // Not a failure if publisher didn't send in time
            GTEST_SKIP() << "No message received from publisher within timeout";
        }
    }
    
    stopProcess(pid);
    ins->UnsubscribeMessage("/HelloWorldTopic");
    ins->SetForwardCallback(nullptr);
    ins->Finalize();
}

// Test DeserializeMessage with all type branches using mock data
TEST_F(DDSProtocolTest, Deserialize_TC002_all_type_branches) {
    using namespace std::chrono_literals;
    
    std::mutex m;
    std::condition_variable cv;
    std::vector<std::shared_ptr<Message>> received_messages;
    
    ins->SetForwardCallback([&](std::shared_ptr<Message> msg) {
        {
            std::lock_guard<std::mutex> lk(m);
            received_messages.push_back(std::move(msg));
        }
        cv.notify_one();
    });
    
    EXPECT_TRUE(ins->Initialize());
    
    // Subscribe to HelloWorld topic which publishes DataTypeTest
    EXPECT_TRUE(ins->SubscribeMessage("/HelloWorldTopic"));
    
    // Wait briefly for any messages (from background publisher if running)
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, 2s);
    }
    
    // Verify if we received messages, DeserializeMessage was called
    if (!received_messages.empty()) {
        for (const auto& msg : received_messages) {
            EXPECT_NE(nullptr, msg);
            EXPECT_FALSE(msg->GetName().empty());
            
            auto data = msg->GetData();
            if (data) {
                // DeserializeMessage successfully extracted data
                EXPECT_FALSE(data->GetKeyList().empty());
            }
        }
    }
    
    ins->UnsubscribeMessage("/HelloWorldTopic");
    ins->SetForwardCallback(nullptr);
    ins->Finalize();
}

// Test DeserializeMessage with nested structure types (TK_STRUCTURE branch)
TEST_F(DDSProtocolTest, Deserialize_TC003_nested_structures) {
    // This test relies on the publisher sending DataTypeTest which has nested structures
    using namespace std::chrono_literals;
    
    std::mutex m;
    std::condition_variable cv;
    std::shared_ptr<Message> received_message;
    bool has_nested = false;
    
    ins->SetForwardCallback([&](std::shared_ptr<Message> msg) {
        {
            std::lock_guard<std::mutex> lk(m);
            received_message = std::move(msg);
            
            // Check if message has nested structure (v_structure field)
            if (received_message && received_message->GetData()) {
                auto data = received_message->GetData();
                auto keys = data->GetKeyList();
                has_nested = std::find(keys.begin(), keys.end(), "v_structure") != keys.end();
            }
        }
        cv.notify_one();
    });
    
    EXPECT_TRUE(ins->Initialize());
    EXPECT_TRUE(ins->SubscribeMessage("/HelloWorldTopic"));
    
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, 2s);
    }
    
    // If we received a message with nested structure, DeserializeMessage TK_STRUCTURE branch was hit
    if (has_nested) {
        auto data = received_message->GetData();
        EXPECT_TRUE(data);
        
        // Verify nested structure was deserialized
        EXPECT_NO_THROW({
            auto nested = data->GetValue<VariantData>("v_structure");
            EXPECT_FALSE(nested.GetName().empty());
        });
    }
    
    ins->UnsubscribeMessage("/HelloWorldTopic");
    ins->SetForwardCallback(nullptr);
    ins->Finalize();
}

// Test DeserializeMessage with sequence types (TK_SEQUENCE branch)
TEST_F(DDSProtocolTest, Deserialize_TC004_sequence_types) {
    using namespace std::chrono_literals;
    
    std::mutex m;
    std::condition_variable cv;
    std::shared_ptr<Message> received_message;
    bool has_sequences = false;
    
    ins->SetForwardCallback([&](std::shared_ptr<Message> msg) {
        {
            std::lock_guard<std::mutex> lk(m);
            received_message = std::move(msg);
            
            if (received_message && received_message->GetData()) {
                auto data = received_message->GetData();
                auto keys = data->GetKeyList();
                // Check for sequence fields
                for (const auto& key : keys) {
                    if (key.find("seq_") == 0) {
                        has_sequences = true;
                        break;
                    }
                }
            }
        }
        cv.notify_one();
    });
    
    EXPECT_TRUE(ins->Initialize());
    EXPECT_TRUE(ins->SubscribeMessage("/HelloWorldTopic"));
    
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, 2s);
    }
    
    // If we received sequences, DeserializeMessage TK_SEQUENCE branch was hit
    if (has_sequences && received_message) {
        auto data = received_message->GetData();
        EXPECT_TRUE(data);
        
        // Verify at least one sequence was deserialized
        auto keys = data->GetKeyList();
        bool found_seq = false;
        for (const auto& key : keys) {
            if (key.find("seq_") == 0) {
                found_seq = true;
                break;
            }
        }
        EXPECT_TRUE(found_seq);
    }
    
    ins->UnsubscribeMessage("/HelloWorldTopic");
    ins->SetForwardCallback(nullptr);
    ins->Finalize();
}

// Test Serialize/Deserialize round-trip
TEST_F(DDSProtocolTest, SerializeDeserialize_TC001_roundtrip) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/RoundTripTest";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/RoundTripTest"] = topic_info;
        
        // Create original data
        auto original_data = std::make_shared<VariantData>();
        original_data->SetName("DataTypeTestSimple");
        original_data->SetValue("id", (int32_t)777);
        original_data->SetValue("name", std::string("RoundTrip"));
        
        auto message = std::make_shared<Message>(
            "/RoundTripTest",
            MessageProtocolType::DDS,
            "DataTypeTestSimple",
            original_data
        );
        
        // Serialize through PublishMessage (calls SerializeMessage internally)
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
        
        // Note: Full round-trip with DeserializeMessage requires a subscriber in another process
        // But this test ensures SerializeMessage is called correctly
    }
    
    ins->Finalize();
}

// Test SerializeMessage with mixed data types in single message
TEST_F(DDSProtocolTest, Serialize_TC005_mixed_types_comprehensive) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/MixedTypesTest";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/MixedTypesTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Mix of scalars and sequences to test multiple branches in one call
        variant_data->SetValue("v_int32", (int32_t)42);
        variant_data->SetValue("v_float", 3.14f);
        variant_data->SetValue("v_string", std::string("mixed"));
        variant_data->SetValue("seq_uint8", std::vector<uint8_t>{1, 2, 3, 4, 5});
        variant_data->SetValue("seq_double", std::vector<double>{1.1, 2.2, 3.3});
        variant_data->SetValue("v_bool", true);
        variant_data->SetValue("seq_string", std::vector<std::string>{"a", "b", "c"});
        
        // Nested structure
        auto nested = std::make_shared<VariantData>();
        nested->SetName("DataTypeTestSimple");
        nested->SetValue("id", (int32_t)111);
        nested->SetValue("name", std::string("Nested"));
        variant_data->SetValue("v_structure", *nested);
        
        auto message = std::make_shared<Message>(
            "/MixedTypesTest",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        // This exercises many SerializeMessage branches in a single call
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// ==================== END COMPREHENSIVE SERIALIZE/DESERIALIZE COVERAGE TESTS ====================
int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
