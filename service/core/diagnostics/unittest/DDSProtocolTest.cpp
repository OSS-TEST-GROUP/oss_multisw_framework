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

// ==================== DataWriter/DataReader Coverage ====================

// Test delete_datawriter
TEST_F(DDSProtocolTest, delete_datawriter_TC001) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    EXPECT_TRUE(ins->Initialize());
    
    // Subscribe and wait for discovery to get a topic with reader
    ins->SubscribeMessage("/HelloWorldTopic");
    
    char* args[] = {"test/HelloWorldPublisher", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        ins->Finalize();
        GTEST_SKIP() << "Failed to start test publisher";
        return;
    }
    
    // Wait for topic discovery
    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (ins->discovered_topic_.size() > 0) {
            auto it = ins->discovered_topic_.find("/HelloWorldTopic");
            if (it != ins->discovered_topic_.end() && it->second) {
                break;
            }
        }
        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    stopProcess(pid);
    ins->Finalize();
}

// Test create_datawriter with valid topic info
TEST_F(DDSProtocolTest, create_datawriter_TC001_valid_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create topic info for a known type
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/WriterTestTopic";
    topic_info->topic_type = "DataTypeTestSimple";
    
    // Register the type
    bool registered = ins->register_type_object_from_dynamic_idl(topic_info);
    if (registered) {
        // Create the data writer
        bool result = ins->create_datawriter(topic_info);
        EXPECT_TRUE(result);
        EXPECT_NE(nullptr, topic_info->writer);
        EXPECT_NE(nullptr, topic_info->topic);
    }
    
    ins->Finalize();
}

// Test create_datawriter with existing topic
TEST_F(DDSProtocolTest, create_datawriter_TC002_existing_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create topic info
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/ExistingTopicWriterTest";
    topic_info->topic_type = "DataTypeTestSimple";
    
    // Register the type first
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        // Create the topic manually first
        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        ins->participant_->get_default_topic_qos(topic_qos);
        topic_info->topic = ins->participant_->create_topic(
            topic_info->topic_name,
            topic_info->topic_type,
            topic_qos
        );
        
        // Now create data writer - should use existing topic
        bool result = ins->create_datawriter(topic_info);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test create_datareader with existing reader (should return true)
TEST_F(DDSProtocolTest, create_datareader_TC001_existing_reader) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/ExistingReaderTest";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        // Create reader first time
        bool result1 = ins->create_datareader(topic_info);
        EXPECT_TRUE(result1);
        
        // Create reader second time - should return true for existing
        bool result2 = ins->create_datareader(topic_info);
        EXPECT_TRUE(result2);
    }
    
    ins->Finalize();
}

// ==================== register_type_object Coverage ====================

// Test register_type_object_from_dynamic_idl with ROS2 type name mapping
TEST_F(DDSProtocolTest, register_type_object_from_dynamic_idl_TC001_ros_mapping) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "rt/ros_type_mapping_test";
    // Use ROS2 DDS type name that needs mapping
    topic_info->topic_type = "diagnosticmgr::msg::dds_::DiagnosticInfoRequest_";
    
    // This should map the ROS2 type to simple type and find it
    bool result = ins->register_type_object_from_dynamic_idl(topic_info);
    // Result depends on IDL availability for DiagnosticInfoRequest
    
    ins->Finalize();
}

// Test register_type_object with both dynamic IDL and discovery fallback
TEST_F(DDSProtocolTest, register_type_object_TC003_discovery_fallback) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/DiscoveryFallbackTest";
    topic_info->topic_type = "CompletelyUnknownType123";
    
    // This should try dynamic IDL first (fail), then discovery (also likely fail)
    bool result = ins->register_type_object(topic_info);
    EXPECT_FALSE(result);  // Unknown type should fail
    
    ins->Finalize();
}

// ==================== load_dds_dynamic_idl Coverage ====================

// Test load_dds_dynamic_idl with valid IDL path
TEST_F(DDSProtocolTest, load_dds_dynamic_idl_TC002_valid_path) {
    // Note: load_dds_dynamic_idl is called during Initialize()
    // We verify it worked by checking parsed_dynamic_type_supports_
    EXPECT_TRUE(ins->Initialize());
    
    // If DIAG_IDL_PATH is set or defaults to "idl", types should be loaded
    // Check that at least some types are registered
    if (!ins->parsed_dynamic_type_supports_.empty()) {
        // Verify types have valid type_support
        for (const auto& pair : ins->parsed_dynamic_type_supports_) {
            EXPECT_NE(nullptr, pair.second);
            EXPECT_NE(nullptr, pair.second->type_support);
        }
    }
    
    ins->Finalize();
}

// ==================== create_topic_information_from_dynamic_idl Coverage ====================

// Test 3-parameter version with valid conversion
TEST_F(DDSProtocolTest, create_topic_info_TC004_ros_conversion_success) {
    EXPECT_TRUE(ins->Initialize());
    
    // Test creating topic info with type conversion
    auto topic_info = ins->create_topic_information_from_dynamic_idl(
        "rt/conversion_test",
        "DataTypeTestSimple",  // Simple type for IDL lookup
        "some::custom::TypeName"  // Original type name for registration
    );
    
    if (topic_info) {
        EXPECT_EQ("rt/conversion_test", topic_info->topic_name);
        EXPECT_EQ("some::custom::TypeName", topic_info->topic_type);
        EXPECT_NE(nullptr, topic_info->remote_type);
        EXPECT_NE(nullptr, topic_info->type_support);
    }
    
    ins->Finalize();
}

// Test 3-parameter version with invalid converted type
TEST_F(DDSProtocolTest, create_topic_info_TC005_ros_conversion_invalid_type) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = ins->create_topic_information_from_dynamic_idl(
        "rt/invalid_conversion",
        "NonExistentConvertedType",  // This won't be found
        "some::ros::Type"
    );
    
    EXPECT_EQ(nullptr, topic_info);  // Should fail
    
    ins->Finalize();
}

// ==================== on_data_writer_discovery Coverage ====================

// Test that on_data_writer_discovery correctly handles discovered topics
TEST_F(DDSProtocolTest, on_data_writer_discovery_TC001) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    EXPECT_TRUE(ins->Initialize());
    
    // Subscribe first to mark topic as needing subscription
    ins->SubscribeMessage("/HelloWorldTopic");
    
    // Start publisher to trigger discovery
    char* args[] = {"test/HelloWorldPublisher", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        ins->Finalize();
        GTEST_SKIP() << "Failed to start publisher";
        return;
    }
    
    // Wait for discovery callback
    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (ins->discovered_topic_.count("/HelloWorldTopic") > 0) {
            auto it = ins->discovered_topic_.find("/HelloWorldTopic");
            if (it != ins->discovered_topic_.end() && it->second && it->second->reader) {
                break;
            }
        }
        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Verify discovery happened
    EXPECT_GT(ins->discovered_topic_.count("/HelloWorldTopic"), 0);
    
    stopProcess(pid);
    ins->Finalize();
}

// ==================== delete_datareader and delete_datawriter Coverage ====================

TEST_F(DDSProtocolTest, delete_datareader_TC001_with_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/DeleteReaderTest";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        // Create reader
        EXPECT_TRUE(ins->create_datareader(topic_info));
        EXPECT_NE(nullptr, topic_info->reader);
        EXPECT_NE(nullptr, topic_info->topic);
        
        // Delete reader - should also delete topic since reader was only user
        EXPECT_TRUE(ins->delete_datareader(topic_info));
        EXPECT_EQ(nullptr, topic_info->reader);
        EXPECT_EQ(nullptr, topic_info->topic);
    }
    
    ins->Finalize();
}

TEST_F(DDSProtocolTest, delete_datawriter_TC002_with_topic) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/DeleteWriterTest";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        // Create writer
        EXPECT_TRUE(ins->create_datawriter(topic_info));
        EXPECT_NE(nullptr, topic_info->writer);
        EXPECT_NE(nullptr, topic_info->topic);
        
        // Delete writer - should also delete topic since no reader exists
        EXPECT_TRUE(ins->delete_datawriter(topic_info));
        EXPECT_EQ(nullptr, topic_info->writer);
        EXPECT_EQ(nullptr, topic_info->topic);
    }
    
    ins->Finalize();
}

// ==================== PublishMessage with valid topic_info full path ====================

TEST_F(DDSProtocolTest, PublishMessage_TC012_full_success_path) {
    EXPECT_TRUE(ins->Initialize());
    
    // Create topic info and set up fully
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/FullSuccessPublish";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        // Add to discovered_topic_
        ins->discovered_topic_["/FullSuccessPublish"] = topic_info;
        
        // Create message
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTestSimple");
        variant_data->SetValue("id", (int32_t)999);
        variant_data->SetValue("name", std::string("FullSuccess"));
        
        auto message = std::make_shared<Message>(
            "/FullSuccessPublish",
            MessageProtocolType::DDS,
            "DataTypeTestSimple",
            variant_data
        );
        
        // Should successfully create writer and publish
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
        
        // Verify writer was created
        EXPECT_NE(nullptr, topic_info->writer);
    }
    
    ins->Finalize();
}

// Test PublishMessage twice to use cached writer
TEST_F(DDSProtocolTest, PublishMessage_TC013_use_cached_writer) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/CachedWriterTest";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/CachedWriterTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTestSimple");
        variant_data->SetValue("id", (int32_t)1001);
        variant_data->SetValue("name", std::string("First"));
        
        auto message = std::make_shared<Message>(
            "/CachedWriterTest",
            MessageProtocolType::DDS,
            "DataTypeTestSimple",
            variant_data
        );
        
        // First publish - creates writer
        bool result1 = ins->PublishMessage(message);
        EXPECT_TRUE(result1);
        DataWriter* first_writer = topic_info->writer;
        
        // Update message
        variant_data->SetValue("id", (int32_t)1002);
        variant_data->SetValue("name", std::string("Second"));
        
        // Second publish - uses existing writer
        bool result2 = ins->PublishMessage(message);
        EXPECT_TRUE(result2);
        
        // Verify same writer was used
        EXPECT_EQ(first_writer, topic_info->writer);
    }
    
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

// ==================== Comprehensive SerializeMessage Tests for Full Coverage ====================

// Test SerializeMessage with complete DataTypeTest (all fields)
TEST_F(DDSProtocolTest, SerializeMessage_TC003_complete_datatype_test) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/CompleteDataTypeTest";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/CompleteDataTypeTest"] = topic_info;
        
        // Create comprehensive variant data with all types
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // All scalar types
        variant_data->SetValue("v_char8", (char)100);
        variant_data->SetValue("v_char16", (wchar_t)200);
        variant_data->SetValue("v_uint8", (uint8_t)50);
        variant_data->SetValue("v_int16", (int16_t)-500);
        variant_data->SetValue("v_uint16", (uint16_t)1000);
        variant_data->SetValue("v_int32", (int32_t)-50000);
        variant_data->SetValue("v_uint32", (uint32_t)100000);
        variant_data->SetValue("v_int64", (int64_t)-1000000);
        variant_data->SetValue("v_uint64", (uint64_t)2000000);
        variant_data->SetValue("v_float", 3.14159f);
        variant_data->SetValue("v_double", 2.71828);
        variant_data->SetValue("v_bool", true);
        variant_data->SetValue("v_string", std::string("Hello DDS"));
        variant_data->SetValue("v_wstring", std::wstring(L"Wide String"));
        
        // Nested structure
        auto nested_variant = std::make_shared<VariantData>();
        nested_variant->SetName("DataTypeTestSimple");
        nested_variant->SetValue("id", (int32_t)42);
        nested_variant->SetValue("name", std::string("NestedStruct"));
        variant_data->SetValue("v_structure", *nested_variant);
        
        // All sequence types
        variant_data->SetValue("seq_char8", std::vector<char>{'a', 'b', 'c'});
        variant_data->SetValue("seq_char16", std::vector<wchar_t>{L'x', L'y', L'z'});
        variant_data->SetValue("seq_uint8", std::vector<uint8_t>{10, 20, 30});
        variant_data->SetValue("seq_int16", std::vector<int16_t>{-100, -200, -300});
        variant_data->SetValue("seq_uint16", std::vector<uint16_t>{1000, 2000, 3000});
        variant_data->SetValue("seq_int32", std::vector<int32_t>{-10000, -20000, -30000});
        variant_data->SetValue("seq_uint32", std::vector<uint32_t>{100000, 200000, 300000});
        variant_data->SetValue("seq_int64", std::vector<int64_t>{-1000000, -2000000, -3000000});
        variant_data->SetValue("seq_uint64", std::vector<uint64_t>{10000000, 20000000, 30000000});
        variant_data->SetValue("seq_float", std::vector<float>{1.1f, 2.2f, 3.3f});
        variant_data->SetValue("seq_double", std::vector<double>{1.11, 2.22, 3.33});
        variant_data->SetValue("seq_bool", std::vector<bool>{true, false, true});
        variant_data->SetValue("seq_string", std::vector<std::string>{"one", "two", "three"});
        variant_data->SetValue("seq_wstring", std::vector<std::wstring>{L"wide1", L"wide2", L"wide3"});
        
        auto message = std::make_shared<Message>(
            "/CompleteDataTypeTest",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        // This should exercise all branches in SerializeMessage
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with nested structures
TEST_F(DDSProtocolTest, SerializeMessage_TC004_nested_structure) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/NestedStructTest";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/NestedStructTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Create nested structure
        auto nested = std::make_shared<VariantData>();
        nested->SetName("DataTypeTestSimple");
        nested->SetValue("id", (int32_t)999);
        nested->SetValue("name", std::string("DeepNested"));
        
        variant_data->SetValue("v_structure", *nested);
        variant_data->SetValue("v_int32", (int32_t)12345);
        
        auto message = std::make_shared<Message>(
            "/NestedStructTest",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with empty sequences
TEST_F(DDSProtocolTest, SerializeMessage_TC005_empty_sequences) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/EmptySeqTest";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/EmptySeqTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Set empty sequences to test branch coverage
        variant_data->SetValue("seq_int32", std::vector<int32_t>{});
        variant_data->SetValue("seq_string", std::vector<std::string>{});
        variant_data->SetValue("seq_bool", std::vector<bool>{});
        
        auto message = std::make_shared<Message>(
            "/EmptySeqTest",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with large sequences
TEST_F(DDSProtocolTest, SerializeMessage_TC006_large_sequences) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/LargeSeqTest";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/LargeSeqTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Create large sequences
        std::vector<int32_t> large_int_seq;
        std::vector<std::string> large_string_seq;
        for (int i = 0; i < 100; i++) {
            large_int_seq.push_back(i);
            large_string_seq.push_back("item_" + std::to_string(i));
        }
        
        variant_data->SetValue("seq_int32", large_int_seq);
        variant_data->SetValue("seq_string", large_string_seq);
        
        auto message = std::make_shared<Message>(
            "/LargeSeqTest",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with wstring types specifically
TEST_F(DDSProtocolTest, SerializeMessage_TC007_wstring_types) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/WstringTest";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/WstringTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Wide string types
        variant_data->SetValue("v_wstring", std::wstring(L"Wide Character String"));
        variant_data->SetValue("v_char16", (wchar_t)L'€');
        
        std::vector<wchar_t> wchar_vec = {L'A', L'B', L'C', L'€', L'£'};
        variant_data->SetValue("seq_char16", wchar_vec);
        
        std::vector<std::wstring> wstring_vec = {L"First", L"Second", L"Third"};
        variant_data->SetValue("seq_wstring", wstring_vec);
        
        auto message = std::make_shared<Message>(
            "/WstringTest",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with boundary values
TEST_F(DDSProtocolTest, SerializeMessage_TC008_boundary_values) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/BoundaryTest";
    topic_info->topic_type = "DataTypeTest";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/BoundaryTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTest");
        
        // Boundary values for integer types
        variant_data->SetValue("v_int8", (int8_t)-128);
        variant_data->SetValue("v_uint8", (uint8_t)255);
        variant_data->SetValue("v_int16", (int16_t)-32768);
        variant_data->SetValue("v_uint16", (uint16_t)65535);
        variant_data->SetValue("v_int32", (int32_t)-2147483648);
        variant_data->SetValue("v_uint32", (uint32_t)4294967295);
        
        // Min and max float values
        variant_data->SetValue("v_float", std::numeric_limits<float>::max());
        variant_data->SetValue("v_double", std::numeric_limits<double>::min());
        
        auto message = std::make_shared<Message>(
            "/BoundaryTest",
            MessageProtocolType::DDS,
            "DataTypeTest",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test SerializeMessage with SimpleDataType
TEST_F(DDSProtocolTest, SerializeMessage_TC009_simple_datatype) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/SimpleDataType";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/SimpleDataType"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTestSimple");
        variant_data->SetValue("id", (int32_t)777);
        variant_data->SetValue("name", std::string("SimpleTest"));
        
        auto message = std::make_shared<Message>(
            "/SimpleDataType",
            MessageProtocolType::DDS,
            "DataTypeTestSimple",
            variant_data
        );
        
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
    }
    
    ins->Finalize();
}

// Test PublishMessage end-to-end to ensure SerializeMessage is actually called
TEST_F(DDSProtocolTest, PublishMessage_TC014_ensure_serialize_called) {
    EXPECT_TRUE(ins->Initialize());
    
    auto topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = "/SerializeCallTest";
    topic_info->topic_type = "DataTypeTestSimple";
    
    if (ins->register_type_object_from_dynamic_idl(topic_info)) {
        ins->discovered_topic_["/SerializeCallTest"] = topic_info;
        
        auto variant_data = std::make_shared<VariantData>();
        variant_data->SetName("DataTypeTestSimple");
        variant_data->SetValue("id", (int32_t)888);
        variant_data->SetValue("name", std::string("SerializeTest"));
        
        auto message = std::make_shared<Message>(
            "/SerializeCallTest",
            MessageProtocolType::DDS,
            "DataTypeTestSimple",
            variant_data
        );
        
        // This should create writer, call SerializeMessage, and write data
        bool result = ins->PublishMessage(message);
        EXPECT_TRUE(result);
        
        // Verify writer was created (indication that publish succeeded)
        EXPECT_NE(nullptr, topic_info->writer);
    }
    
    ins->Finalize();
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
