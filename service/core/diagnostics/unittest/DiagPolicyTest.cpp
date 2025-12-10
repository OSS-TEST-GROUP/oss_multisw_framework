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

TEST_F(DiagPolicyTest, InactivePolicy_TC005) {
    DiagPolicy inactive_policy(99, "InactivePolicy", false, rules);
    EXPECT_FALSE(inactive_policy.IsActive());
}

TEST_F(DiagPolicyTest, EmptyRules_TC006) {
    std::vector<DiagPolicyRule> empty_rules;
    DiagPolicy empty_policy(100, "EmptyRulesPolicy", true, empty_rules);
    EXPECT_EQ(empty_policy.GetRules().size(), 0);
}

TEST_F(DiagPolicyTest, SingleRule_TC007) {
    std::vector<DiagPolicyRule> single_rule = {
        MakeDummyRule(DiagInfoStatus::FATAL, "Critical error", {"cond1"}, ConditionOperator::AND)
    };
    DiagPolicy single_rule_policy(200, "SingleRulePolicy", true, single_rule);
    auto retrieved_rules = single_rule_policy.GetRules();
    ASSERT_EQ(retrieved_rules.size(), 1);
    EXPECT_EQ(retrieved_rules[0].message, "Critical error");
}

TEST_F(DiagPolicyTest, MultipleRules_TC008) {
    std::vector<DiagPolicyRule> multiple_rules = {
        MakeDummyRule(DiagInfoStatus::PASS, "OK", {"c1"}, ConditionOperator::AND),
        MakeDummyRule(DiagInfoStatus::WARNING, "Warn", {"c2"}, ConditionOperator::OR),
        MakeDummyRule(DiagInfoStatus::FATAL, "Error", {"c3"}, ConditionOperator::AND),
        MakeDummyRule(DiagInfoStatus::FATAL, "Fail", {"c4"}, ConditionOperator::OR)
    };
    DiagPolicy multi_policy(300, "MultiRulePolicy", true, multiple_rules);
    auto retrieved_rules = multi_policy.GetRules();
    EXPECT_EQ(retrieved_rules.size(), 4);
}

TEST_F(DiagPolicyTest, EmptyName_TC009) {
    DiagPolicy empty_name_policy(400, "", true, rules);
    EXPECT_EQ(empty_name_policy.GetDiagName(), "");
}

TEST_F(DiagPolicyTest, LongName_TC010) {
    std::string long_name(1000, 'x');
    DiagPolicy long_name_policy(500, long_name, true, rules);
    EXPECT_EQ(long_name_policy.GetDiagName(), long_name);
}

TEST_F(DiagPolicyTest, SpecialCharactersInName_TC011) {
    DiagPolicy special_policy(600, "Policy-Name_123.test!@#", true, rules);
    EXPECT_EQ(special_policy.GetDiagName(), "Policy-Name_123.test!@#");
}

TEST_F(DiagPolicyTest, NegativeCode_TC012) {
    DiagPolicy negative_policy(-100, "NegativeCodePolicy", true, rules);
    EXPECT_EQ(negative_policy.GetDiagCode(), -100);
}

TEST_F(DiagPolicyTest, ZeroCode_TC013) {
    DiagPolicy zero_policy(0, "ZeroCodePolicy", true, rules);
    EXPECT_EQ(zero_policy.GetDiagCode(), 0);
}

TEST_F(DiagPolicyTest, LargeCode_TC014) {
    DiagPolicy large_policy(0x7FFFFFFF, "LargeCodePolicy", true, rules);
    EXPECT_EQ(large_policy.GetDiagCode(), 0x7FFFFFFF);
}

TEST_F(DiagPolicyTest, AllConditionOperators_TC015) {
    std::vector<DiagPolicyRule> operator_rules = {
        MakeDummyRule(DiagInfoStatus::PASS, "AND rule", {"c1", "c2"}, ConditionOperator::AND),
        MakeDummyRule(DiagInfoStatus::FATAL, "OR rule", {"c3", "c4"}, ConditionOperator::OR)
    };
    DiagPolicy operator_policy(700, "OperatorPolicy", true, operator_rules);
    EXPECT_EQ(operator_policy.GetRules().size(), 2);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
