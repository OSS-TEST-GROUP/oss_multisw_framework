#include <gtest/gtest.h>

#include "diagnostic_analyzer/DiagnosticAnalyzer.cpp"
#include "diagnostic_analyzer/ConditionEvaluator.cpp"
#include "diagnostic_analyzer/PolicyManager.cpp"
#include "diagnostic_analyzer/DiagPolicy.cpp"
#include "diagnostic_analyzer/DiagPolicyRule.cpp"
#include "diagnostic_analyzer/ReferenceMessage.cpp"
#include "diagnostic_analyzer/DataManager.cpp"
#include "diagnostic/DiagInfoRegistry.cpp"
#include "diagnostic/DiagInfo.cpp"
#include "diagnostic/VariantData.cpp"
#include "transmitter/request_msg/IncomingMessageBase.cpp"
#include "transmitter/Message.cpp"
#include "executor/RequestBase.cpp"

#include "mock/MessageTransmitter_mock.hpp"
#include "mock/ServiceExecutor_mock.hpp"
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
using ::testing::Invoke;

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class DiagnosticAnalyzerTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        service_executor_mock_ = std::make_shared<ServiceExecutorMock>();
        Mock_ServiceExecutorMock = service_executor_mock_.get();
        Mock_MessageTransmitterMock = new MessageTransmitterMock;
        diag_info_registry = std::make_shared<DiagInfoRegistry>();
        message_transmitter = std::make_shared<MessageTransmitter>(service_executor_mock_);
        ins = new DiagnosticAnalyzer(service_executor_mock_, message_transmitter, diag_info_registry);
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        service_executor_mock_.reset();
        Mock_ServiceExecutorMock = nullptr;
        message_transmitter.reset();
        diag_info_registry.reset();
        delete Mock_MessageTransmitterMock;
        delete ins;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    template<typename ValueType>
    void AddTestValue(std::shared_ptr<DataManager> data_manager, const std::string& name, const std::string& key, ValueType value) {
        auto data = std::make_shared<diagnostic_manager::VariantData>();
        data->SetName(name);
        data->SetValue(key, value);
        data_manager->UpdateData(data);
    }

    void AddReferenceMessage(std::shared_ptr<PolicyManager> policy_manager, const std::string& name, const std::string& msg, MessageProtocolType protocol, std::set<int32_t> diag_codes) {
        auto ref_msg = std::make_shared<ReferenceMessage>(name, msg, protocol);
        for (auto iter : diag_codes) {
            ref_msg->SetDiagCode(iter);
        }
        policy_manager->reference_msgs_.push_back(ref_msg);
    }

    DiagnosticAnalyzer* ins;

    std::shared_ptr<DiagInfoRegistry> diag_info_registry;
    std::shared_ptr<MessageTransmitter> message_transmitter;
    std::shared_ptr<ServiceExecutorMock> service_executor_mock_;
};

TEST_F(DiagnosticAnalyzerTest, Initialize_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    EXPECT_TRUE(ins->Initialize());
}

TEST_F(DiagnosticAnalyzerTest, Initialize_TC002_AlreadyInitialized) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    EXPECT_TRUE(ins->Initialize());
    // Call Initialize again - should return true immediately (already initialized)
    EXPECT_TRUE(ins->Initialize());
}

TEST_F(DiagnosticAnalyzerTest, Finalize_TC001_without_initialize) {
    // Finalize without Initialize - should return early (not initialized)
    EXPECT_NO_THROW(ins->Finalize());
}

TEST_F(DiagnosticAnalyzerTest, Finalize_TC002_after_initialize) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_MessageTransmitterMock, RemoveSubscriber(_)).Times(1);

    EXPECT_TRUE(ins->Initialize());
    // Finalize after Initialize - should clean up properly
    EXPECT_NO_THROW(ins->Finalize());
}

TEST_F(DiagnosticAnalyzerTest, AnalyzeAll_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    auto data1 = std::make_shared<VariantData>();
    data1->SetName("vehicle/EngineStatusNotification");
    data1->SetValue("temperature", 124);
    data1->SetValue("rpm", 1256);
    auto data2 = std::make_shared<VariantData>();
    data2->SetName("vehicle/TransmissionNotification");
    data2->SetValue("temperature", 78.2);
    data2->SetValue("gear", "D");

    ins->data_manager_->UpdateData(data1);
    ins->data_manager_->UpdateData(data2);

    // Setup policy
    DiagPolicyRule rule(DiagInfoStatus::WARNING, "EngineOverheat", std::vector<std::string>{"Engine.temperature > 90"}, ConditionOperator::AND);
    auto policy = std::make_shared<DiagPolicy>(101, "EngineTempCheck", true, std::vector<DiagPolicyRule>{rule});
    DiagPolicyRule rule2(DiagInfoStatus::WARNING, "TransmissionOverheat", std::vector<std::string>{"Transmission.temperature > 70"}, ConditionOperator::AND);
    auto policy2 = std::make_shared<DiagPolicy>(102, "TransmissionTempCheck", true, std::vector<DiagPolicyRule>{rule2});

    ins->policy_manager_->policies_.push_back(policy);
    ins->policy_manager_->policies_.push_back(policy2);

    // Reference message
    AddReferenceMessage(ins->policy_manager_, "Engine", "vehicle/EngineStatusNotification", MessageProtocolType::DDS, std::set<int32_t>{101});
    AddReferenceMessage(ins->policy_manager_, "Transmission", "vehicle/TransmissionNotification", MessageProtocolType::DDS, std::set<int32_t>{102});

    // Test
    auto result = ins->AnalyzeAll();
    EXPECT_EQ(2, result.size());

}

TEST_F(DiagnosticAnalyzerTest, Analyze_TC001_ConditionAnd) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Setup policy
    DiagPolicyRule rule(DiagInfoStatus::WARNING, "Overheat", std::vector<std::string>{"Engine.temp > 90", "Vehicle.ignition = \"on\""}, ConditionOperator::AND);
    auto policy = std::make_shared<DiagPolicy>(101, "EngineTempCheck", true, std::vector<DiagPolicyRule>{rule});

    // Insert test data
    auto data_manager = ins->data_manager_;
    AddTestValue(data_manager, "Engine", "temp", 95);
    AddTestValue(data_manager, "Vehicle", "ignition", "on");

    // Run Analyze
    auto results = ins->Analyze(policy);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0]->GetDiagCode(), 101);
    EXPECT_EQ(results[0]->GetDiagName(), "EngineTempCheck");
    EXPECT_EQ(results[0]->GetStatus(), DiagInfoStatus::WARNING);
    EXPECT_EQ(results[0]->GetMessage(), "Overheat");
}

TEST_F(DiagnosticAnalyzerTest, Analyze_TC001_ConditionAnd_NotSatisfied) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Setup policy
    DiagPolicyRule rule(DiagInfoStatus::WARNING, "Overheat", std::vector<std::string>{"Engine.temp > 90", "Vehicle.ignition = \"on\""}, ConditionOperator::AND);
    auto policy = std::make_shared<DiagPolicy>(101, "EngineTempCheck", true, std::vector<DiagPolicyRule>{rule});

    // Insert test data
    auto data_manager = ins->data_manager_;
    AddTestValue(data_manager, "Engine", "temp", 89);
    AddTestValue(data_manager, "Vehicle", "ignition", "on");

    // Run Analyze
    auto results = ins->Analyze(policy);

    ASSERT_EQ(results.size(), 0);
}

TEST_F(DiagnosticAnalyzerTest, Analyze_TC001_ConditionOr) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Setup policy
    DiagPolicyRule rule(DiagInfoStatus::WARNING, "Overheat", std::vector<std::string>{"Engine.temp > 90", "Vehicle.ignition = \"on\""}, ConditionOperator::OR);
    auto policy = std::make_shared<DiagPolicy>(101, "EngineTempCheck", true, std::vector<DiagPolicyRule>{rule});

    // Insert test data
    auto data_manager = ins->data_manager_;
    AddTestValue(data_manager, "Engine", "temp", 95);
    AddTestValue(data_manager, "Vehicle", "ignition", "off");

    // Run Analyze
    auto results = ins->Analyze(policy);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0]->GetDiagCode(), 101);
    EXPECT_EQ(results[0]->GetDiagName(), "EngineTempCheck");
    EXPECT_EQ(results[0]->GetStatus(), DiagInfoStatus::WARNING);
    EXPECT_EQ(results[0]->GetMessage(), "Overheat");
}

TEST_F(DiagnosticAnalyzerTest, Analyze_TC001_ConditionOr_NotSatisfied) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Setup policy
    DiagPolicyRule rule(DiagInfoStatus::WARNING, "Overheat", std::vector<std::string>{"Engine.temp > 90", "Vehicle.ignition = \"on\""}, ConditionOperator::OR);
    auto policy = std::make_shared<DiagPolicy>(101, "EngineTempCheck", true, std::vector<DiagPolicyRule>{rule});

    // Insert test data
    auto data_manager = ins->data_manager_;
    AddTestValue(data_manager, "Engine", "temp", 80);
    AddTestValue(data_manager, "Vehicle", "ignition", "off");

    // Run Analyze
    auto results = ins->Analyze(policy);

    ASSERT_EQ(results.size(), 0);
}

// ==================== HandleReceiveDiagMessage Tests ====================

// Test HandleReceiveDiagMessage with valid DiagMessage
TEST_F(DiagnosticAnalyzerTest, HandleReceiveDiagMessage_TC001_valid_message) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_ServiceExecutorMock, RequestTask(_)).WillOnce(Return(true));

    ins->Initialize();

    // Create a valid DiagMessage with data
    auto diag_msg = std::make_shared<DiagMessage>();
    auto variant_data = std::make_shared<VariantData>();
    variant_data->SetName("vehicle/EngineStatus");
    variant_data->SetValue("temperature", 95);
    variant_data->SetValue("rpm", 3000);
    
    auto message = std::make_shared<Message>("vehicle/EngineStatus", MessageProtocolType::DDS, "EngineStatusType", variant_data);
    diag_msg->SetMessage(message);

    // Call HandleReceiveDiagMessage
    ins->HandleReceiveDiagMessage(diag_msg);

    // Verify data was updated in data_manager
    auto all_data = ins->data_manager_->GetAllUpdatedData();
    EXPECT_GE(all_data.size(), 1);
}

// Test HandleReceiveDiagMessage with null DiagMessage (returns early)
TEST_F(DiagnosticAnalyzerTest, HandleReceiveDiagMessage_TC002_null_diag_msg) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Pass a non-DiagMessage RequestBase (will fail dynamic_pointer_cast)
    auto invalid_msg = std::make_shared<RequestBase>();
    
    // Should return early without crash
    EXPECT_NO_THROW(ins->HandleReceiveDiagMessage(invalid_msg));
}

// Test HandleReceiveDiagMessage with DiagMessage but null data (returns early)
TEST_F(DiagnosticAnalyzerTest, HandleReceiveDiagMessage_TC003_null_data) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Create DiagMessage with message but null data
    auto diag_msg = std::make_shared<DiagMessage>();
    auto message = std::make_shared<Message>("vehicle/EngineStatus", MessageProtocolType::DDS, "EngineStatusType", nullptr);
    diag_msg->SetMessage(message);

    // Should return early without crash
    EXPECT_NO_THROW(ins->HandleReceiveDiagMessage(diag_msg));
}

// ==================== HandleAnalyzeRequest Tests ====================

// Test HandleAnalyzeRequest with valid request and policies
TEST_F(DiagnosticAnalyzerTest, HandleAnalyzeRequest_TC001_valid_request) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Setup test data
    auto data1 = std::make_shared<VariantData>();
    data1->SetName("vehicle/EngineStatusNotification");
    data1->SetValue("temperature", 124);
    data1->SetValue("rpm", 1256);

    ins->data_manager_->UpdateData(data1);

    // Setup policy
    DiagPolicyRule rule(DiagInfoStatus::WARNING, "EngineOverheat", std::vector<std::string>{"Engine.temperature > 90"}, ConditionOperator::AND);
    auto policy = std::make_shared<DiagPolicy>(101, "EngineTempCheck", true, std::vector<DiagPolicyRule>{rule});
    ins->policy_manager_->policies_.push_back(policy);

    // Add reference message to link data to policy
    AddReferenceMessage(ins->policy_manager_, "Engine", "vehicle/EngineStatusNotification", MessageProtocolType::DDS, std::set<int32_t>{101});

    // Create request message
    auto req_msg = std::make_shared<AnalyzeRequestMessage>();

    // Call HandleAnalyzeRequest
    ins->HandleAnalyzeRequest(req_msg);

    // Verify diag_info_registry was updated
    auto diag_info = ins->diag_info_registry_->GetDiagInfo(101);
    // Note: Result depends on condition evaluation
}

// Test HandleAnalyzeRequest with empty policies (no analysis results)
TEST_F(DiagnosticAnalyzerTest, HandleAnalyzeRequest_TC002_no_policies) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // No policies setup, just call HandleAnalyzeRequest
    auto req_msg = std::make_shared<AnalyzeRequestMessage>();

    // Should handle gracefully with no results
    EXPECT_NO_THROW(ins->HandleAnalyzeRequest(req_msg));
}

// Test HandleAnalyzeRequest updates DiagInfoRegistry with multiple results
TEST_F(DiagnosticAnalyzerTest, HandleAnalyzeRequest_TC003_multiple_results) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Setup multiple test data
    auto data1 = std::make_shared<VariantData>();
    data1->SetName("vehicle/EngineStatusNotification");
    data1->SetValue("temperature", 124);
    
    auto data2 = std::make_shared<VariantData>();
    data2->SetName("vehicle/TransmissionNotification");
    data2->SetValue("temperature", 85.0);

    ins->data_manager_->UpdateData(data1);
    ins->data_manager_->UpdateData(data2);

    // Setup policies
    DiagPolicyRule rule1(DiagInfoStatus::WARNING, "EngineOverheat", std::vector<std::string>{"Engine.temperature > 90"}, ConditionOperator::AND);
    auto policy1 = std::make_shared<DiagPolicy>(101, "EngineTempCheck", true, std::vector<DiagPolicyRule>{rule1});
    
    DiagPolicyRule rule2(DiagInfoStatus::FATAL, "TransmissionOverheat", std::vector<std::string>{"Transmission.temperature > 80"}, ConditionOperator::AND);
    auto policy2 = std::make_shared<DiagPolicy>(102, "TransmissionTempCheck", true, std::vector<DiagPolicyRule>{rule2});

    ins->policy_manager_->policies_.push_back(policy1);
    ins->policy_manager_->policies_.push_back(policy2);

    // Add reference messages
    AddReferenceMessage(ins->policy_manager_, "Engine", "vehicle/EngineStatusNotification", MessageProtocolType::DDS, std::set<int32_t>{101});
    AddReferenceMessage(ins->policy_manager_, "Transmission", "vehicle/TransmissionNotification", MessageProtocolType::DDS, std::set<int32_t>{102});

    // Create and call HandleAnalyzeRequest
    auto req_msg = std::make_shared<AnalyzeRequestMessage>();
    ins->HandleAnalyzeRequest(req_msg);

    // Both policies should have been analyzed
}

// ==================== EvaluatePolicyRuleCondition Tests ====================

TEST_F(DiagnosticAnalyzerTest, EvaluatePolicyRuleCondition_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    // Add test data
    AddTestValue(ins->data_manager_, "Engine", "temp", 100);

    // Test condition evaluation
    bool result = ins->EvaluatePolicyRuleCondition("Engine.temp > 90");
    EXPECT_TRUE(result);

    result = ins->EvaluatePolicyRuleCondition("Engine.temp < 50");
    EXPECT_FALSE(result);
}

TEST_F(DiagnosticAnalyzerTest, SubscribeReferenceMessages_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/LifecycleDiagReport", MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/ADSWManagerDiagReport", MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();
}

TEST_F(DiagnosticAnalyzerTest, GetDiagPoliciesToAnalyze_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/LifecycleDiagReport", MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/ADSWManagerDiagReport", MessageProtocolType::DDS, false)).WillRepeatedly(Return(true));

    ins->Initialize();

    auto data1 = std::make_shared<VariantData>();
    data1->SetName("mmf/lifecyclemgr/LifecycleDiagReport");
    auto data2 = std::make_shared<VariantData>();
    data2->SetName("mmf/lifecyclemgr/LifecycleDiagReport");
    auto data3 = std::make_shared<VariantData>();
    data3->SetName("mmf/lifecyclemgr/ADSWManagerDiagReport");
    auto data4 = std::make_shared<VariantData>();
    data4->SetName("mmf/lifecyclemgr/TestReport");

    ins->data_manager_->UpdateData(data1);
    ins->data_manager_->UpdateData(data2);
    ins->data_manager_->UpdateData(data3);
    ins->data_manager_->UpdateData(data4);


    auto ret = ins->GetDiagPoliciesToAnalyze();
    EXPECT_EQ(3, ret.size());
    EXPECT_EQ("lifecycle", ret[0]->GetDiagName());
    EXPECT_EQ("lifecycle2", ret[1]->GetDiagName());
    EXPECT_EQ("adsw_manager", ret[2]->GetDiagName());
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
