#include <gtest/gtest.h>

#include "diagnostic_analyzer/DiagnosticAnalyzer.cpp"
#include "diagnostic_analyzer/ConditionEvaluator.cpp"
#include "diagnostic_analyzer/PolicyManager.cpp"
#include "diagnostic_analyzer/DiagPolicy.cpp"
#include "diagnostic_analyzer/DiagPolicyRule.cpp"
#include "diagnostic_analyzer/ReferenceMessage.cpp"
#include "diagnostic_analyzer/DataManager.cpp"
#include "diagnostic/DiagInfo.cpp"
#include "diagnostic/VariantData.cpp"
#include "transmitter/request_msg/IncomingMessageBase.cpp"
#include "transmitter/Message.cpp"
#include "executor/RequestBase.cpp"

#include "mock/MessageTransmitter_mock.hpp"
#include "mock/ServiceExecutor_mock.hpp"

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
        message_transmitter = std::make_shared<MessageTransmitter>(service_executor_mock_);
        ins = new DiagnosticAnalyzer(service_executor_mock_, message_transmitter);
    }
    virtual void TearDown(){
        service_executor_mock_.reset();
        Mock_ServiceExecutorMock = nullptr;
        message_transmitter.reset();
        delete Mock_MessageTransmitterMock;
        delete ins;
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

    std::shared_ptr<MessageTransmitter> message_transmitter;
    std::shared_ptr<ServiceExecutorMock> service_executor_mock_;
};

TEST_F(DiagnosticAnalyzerTest, Initialize_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, Finalize_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, AnalyzeAll_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS)).WillRepeatedly(Return(true));

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
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS)).WillRepeatedly(Return(true));

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
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS)).WillRepeatedly(Return(true));

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
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS)).WillRepeatedly(Return(true));

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
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, _, MessageProtocolType::DDS)).WillRepeatedly(Return(true));

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

TEST_F(DiagnosticAnalyzerTest, HandleReceiveDiagMessage_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, HandleAnalyzeRequest_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, EvaluatePolicyRuleCondition_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, SubscribeReferenceMessages_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/LifecycleDiagReport", MessageProtocolType::DDS)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/ADSWManagerDiagReport", MessageProtocolType::DDS)).WillRepeatedly(Return(true));

    ins->Initialize();
}

TEST_F(DiagnosticAnalyzerTest, GetDiagPoliciesToAnalyze_TC001) {
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillOnce(Return(1234));
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/LifecycleDiagReport", MessageProtocolType::DDS)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/ADSWManagerDiagReport", MessageProtocolType::DDS)).WillRepeatedly(Return(true));

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
