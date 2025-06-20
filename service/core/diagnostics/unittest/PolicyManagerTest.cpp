#include <gtest/gtest.h>

#include "diagnostic_analyzer/PolicyManager.cpp"
#include "diagnostic_analyzer/DiagPolicy.cpp"
#include "diagnostic_analyzer/DiagPolicyRule.cpp"
#include "diagnostic_analyzer/ReferenceMessage.cpp"

using namespace diagnostic_manager;
using namespace std::chrono_literals;

namespace fs = std::filesystem;
using namespace diagnostic_manager;

class PolicyManagerTest : public ::testing::Test {
protected:
    const std::string config_path = "config";

    void SetUp() override {
        ins = new PolicyManager;
    }

    void TearDown() override {
        delete ins;
    }

    PolicyManager* ins;
};

TEST_F(PolicyManagerTest, Initialize_TC001) {
    PolicyManager manager;
    ins->LoadAllPolicy(config_path);
    ASSERT_GT(ins->GetPolicyNum(), 0);

    ins->Initialize();
    EXPECT_EQ(ins->GetPolicyNum(), 0);
    EXPECT_EQ(ins->GetReferenceMessageNum(), 0);
}

TEST_F(PolicyManagerTest, LoadAllPolicies_TC001) {
    ASSERT_TRUE(ins->LoadAllPolicy(config_path));
    EXPECT_EQ(ins->GetPolicyNum(), 3);
    EXPECT_EQ(ins->GetReferenceMessageNum(), 2);

    // Validate policies that read from test config.
    auto lifecycle = ins->GetPolicy(10);
    ASSERT_NE(lifecycle, nullptr);
    EXPECT_EQ(lifecycle->GetDiagName(), "lifecycle");
    EXPECT_TRUE(lifecycle->IsActive());

    auto lifecycle_rules = lifecycle->GetRules();
    ASSERT_EQ(lifecycle_rules.size(), 2);

    EXPECT_EQ(lifecycle_rules[0].status, DiagInfoStatus::FATAL);  // "FATAL" maps to FATAL
    EXPECT_EQ(lifecycle_rules[0].message, "At least one service is not alive.");
    EXPECT_EQ(lifecycle_rules[0].conditions.size(), 1);
    EXPECT_EQ(lifecycle_rules[0].conditions[0], "LifecycleDiagReport.service_list[].service_state not in [1,2]");
    EXPECT_EQ(lifecycle_rules[0].condition_operator, ConditionOperator::AND);

    EXPECT_EQ(lifecycle_rules[1].status, DiagInfoStatus::WARNING);
    EXPECT_EQ(lifecycle_rules[1].message, "AD SW is not initizlied yet.");
    EXPECT_EQ(lifecycle_rules[1].conditions.size(), 2);
    EXPECT_EQ(lifecycle_rules[1].conditions[0], "LifecycleDiagReport.initialized = false");
    EXPECT_EQ(lifecycle_rules[1].conditions[1], "LifecycleDiagReport.isConnected = false");
    EXPECT_EQ(lifecycle_rules[1].condition_operator, ConditionOperator::AND);

    // Validate DiagPolicy: adsw_manager
    auto adsw = ins->GetPolicy(12);
    ASSERT_NE(adsw, nullptr);
    EXPECT_EQ(adsw->GetDiagName(), "adsw_manager");
    EXPECT_TRUE(adsw->IsActive());

    auto adsw_rules = adsw->GetRules();
    ASSERT_EQ(adsw_rules.size(), 1);
    EXPECT_EQ(adsw_rules[0].status, DiagInfoStatus::FATAL);
    EXPECT_EQ(adsw_rules[0].message, "AD SW Manager can't connect to Lifecycle Manager.");
    ASSERT_EQ(adsw_rules[0].conditions.size(), 1);
    EXPECT_EQ(adsw_rules[0].conditions[0], "ADSWManagerDiagReport.isConnected = false");

    // Validate ReferenceMessage: LifecycleDiagReport
    auto ref1 = ins->GetReferenceMessage("LifecycleDiagReport");
    ASSERT_NE(ref1, nullptr);
    EXPECT_EQ(ref1->GetMessage(), "mmf/lifecyclemgr/LifecycleDiagReport");
    EXPECT_EQ(ref1->GetProtocol(), MessageProtocolType::DDS);
    EXPECT_TRUE(ref1->CheckDiagCode(10));
    EXPECT_TRUE(ref1->CheckDiagCode(11));
    EXPECT_FALSE(ref1->CheckDiagCode(12));

    // Validate ReferenceMessage: ADSWManagerDiagReport
    auto ref2 = ins->GetReferenceMessage("ADSWManagerDiagReport");
    ASSERT_NE(ref2, nullptr);
    EXPECT_EQ(ref2->GetMessage(), "mmf/lifecyclemgr/ADSWManagerDiagReport");
    EXPECT_EQ(ref2->GetProtocol(), MessageProtocolType::DDS);
    EXPECT_FALSE(ref2->CheckDiagCode(10));
    EXPECT_FALSE(ref2->CheckDiagCode(11));
    EXPECT_TRUE(ref2->CheckDiagCode(12));
}

TEST_F(PolicyManagerTest, GetPolicy_TC001) {
    auto policy = std::make_shared<DiagPolicy>(10, "engine", true, std::vector<DiagPolicyRule>());
    ins->policies_.push_back(policy);

    auto result = ins->GetPolicy(10);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiagCode(), 10);
}

TEST_F(PolicyManagerTest, GetPolicyByIndex_TC001) {
    auto policy = std::make_shared<DiagPolicy>(20, "brakes", true, std::vector<DiagPolicyRule>());
    ins->policies_.push_back(policy);

    auto result = ins->GetPolicyByIndex(0);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiagName(), "brakes");
}

TEST_F(PolicyManagerTest, GetPolicyByName_TC001) {
    auto policy = std::make_shared<DiagPolicy>(30, "steering", false, std::vector<DiagPolicyRule>());
    ins->policies_.push_back(policy);

    auto result = ins->GetPolicyByName("steering");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiagCode(), 30);
}

TEST_F(PolicyManagerTest, GetReferenceMessage_TC001) {
    auto refMsg = std::make_shared<ReferenceMessage>(
        "SensorReport",
        "vehicle/sensors/SensorReport",
        MessageProtocolType::DDS
    );
    ins->reference_msgs_.push_back(refMsg);

    auto result = ins->GetReferenceMessage("SensorReport");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetMessage(), "vehicle/sensors/SensorReport");
}

TEST_F(PolicyManagerTest, GetAllReferenceMessage_TC001) {
    ins->reference_msgs_.push_back(std::make_shared<ReferenceMessage>(
        "GPS", "nav/GPS", MessageProtocolType::DDS));
    ins->reference_msgs_.push_back(std::make_shared<ReferenceMessage>(
        "IMU", "nav/IMU", MessageProtocolType::DDS));

    auto result = ins->GetAllReferenceMessage();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0]->GetName(), "GPS");
    EXPECT_EQ(result[1]->GetName(), "IMU");
}

TEST_F(PolicyManagerTest, GetPolicyNum_TC001) {
    ins->policies_.push_back(std::make_shared<DiagPolicy>(1, "p1", true, std::vector<DiagPolicyRule>()));
    ins->policies_.push_back(std::make_shared<DiagPolicy>(2, "p2", true, std::vector<DiagPolicyRule>()));

    EXPECT_EQ(ins->GetPolicyNum(), 2);
}

TEST_F(PolicyManagerTest, GetReferenceMessageNum_TC001) {
    ins->reference_msgs_.push_back(std::make_shared<ReferenceMessage>(
        "A", "m/a", MessageProtocolType::DDS));
    ins->reference_msgs_.push_back(std::make_shared<ReferenceMessage>(
        "B", "m/b", MessageProtocolType::DDS));
    ins->reference_msgs_.push_back(std::make_shared<ReferenceMessage>(
        "C", "m/c", MessageProtocolType::DDS));

    EXPECT_EQ(ins->GetReferenceMessageNum(), 3);
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
