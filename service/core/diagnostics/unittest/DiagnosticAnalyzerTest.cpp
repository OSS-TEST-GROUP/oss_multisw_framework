#include <gtest/gtest.h>

#include "diagnostic_analyzer/DiagnosticAnalyzer.cpp"
#include "diagnostic_analyzer/ConditionEvaluator.cpp"
#include "diagnostic_analyzer/PolicyManager.cpp"
#include "diagnostic_analyzer/DiagPolicy.cpp"
#include "diagnostic_analyzer/DiagPolicyRule.cpp"
#include "diagnostic_analyzer/ReferenceMessage.cpp"
#include "diagnostic_analyzer/DataManager.cpp"
#include "diagnostic/VariantData.cpp"
#include "transmitter/request_msg/IncomingMessageBase.cpp"
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
        ins = new DiagnosticAnalyzer(message_transmitter);
    }
    virtual void TearDown(){
        service_executor_mock_.reset();
        Mock_ServiceExecutorMock = nullptr;
        message_transmitter.reset();
        delete Mock_MessageTransmitterMock;
        delete ins;
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
}

TEST_F(DiagnosticAnalyzerTest, Analyze_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, HandleReceiveDiagMessage_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, HandleAnalyzeRequest_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, EvaluatePolicyRuleCondition_TC001) {
}

TEST_F(DiagnosticAnalyzerTest, SubscribeReferenceMessages_TC001) {
    EXPECT_CALL(*Mock_MessageTransmitterMock, AddSubscriber("DiagnosticAnalyzer", _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/LifecycleDiagReport", MessageProtocolType::DDS)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_MessageTransmitterMock, SubscribeMessage(_, "mmf/lifecyclemgr/ADSWManagerDiagReport", MessageProtocolType::DDS)).WillRepeatedly(Return(true));

    ins->Initialize();
}

TEST_F(DiagnosticAnalyzerTest, GetDiagPoliciesToAnalyze_TC001) {
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
