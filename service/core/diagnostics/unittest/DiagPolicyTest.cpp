#include <gtest/gtest.h>

#include "diagnostic_analyzer/DiagPolicy.cpp"
#include "diagnostic_analyzer/DiagPolicyRule.cpp"

using namespace diagnostic_manager;

class DiagPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        rules = {
            MakeDummyRule(DiagInfoStatus::PASS, "All good", {"cond1"}, ConditionOperator::AND),
            MakeDummyRule(DiagInfoStatus::WARNING, "Voltage high", {"cond2", "cond3"}, ConditionOperator::AND)
        };

        policy = new DiagPolicy(42, "VoltagePolicy", true, rules);
    }

    void TearDown() override {
        delete policy;
    }

    DiagPolicyRule MakeDummyRule(DiagInfoStatus status, const std::string& msg, const std::vector<std::string>& conditions, ConditionOperator condition_operator) {
        return DiagPolicyRule(status, msg, conditions, condition_operator);
    }

    DiagPolicy* policy;
    std::vector<DiagPolicyRule> rules;
};

// Tests

TEST_F(DiagPolicyTest, GetDiagCode_TC001) {
    EXPECT_EQ(policy->GetDiagCode(), 42);
}

TEST_F(DiagPolicyTest, GetDiagName_TC002) {
    EXPECT_EQ(policy->GetDiagName(), "VoltagePolicy");
}

TEST_F(DiagPolicyTest, IsActive_TC003) {
    EXPECT_TRUE(policy->IsActive());
}

TEST_F(DiagPolicyTest, GetRules_TC004) {
    auto retrieved_rules = policy->GetRules();
    ASSERT_EQ(retrieved_rules.size(), 2);
    EXPECT_EQ(retrieved_rules[0].message, "All good");
    EXPECT_EQ(retrieved_rules[1].message, "Voltage high");
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
