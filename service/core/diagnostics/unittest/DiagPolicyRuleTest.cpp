#include <gtest/gtest.h>

#include "diagnostic_analyzer/DiagPolicyRule.cpp"

using namespace diagnostic_manager;

class DiagPolicyRuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        conditions = {"cond_1", "cond_2"};
        rule = new DiagPolicyRule(DiagInfoStatus::WARNING, "Voltage too high", conditions, ConditionOperator::AND);
    }

    void TearDown() override {
        delete rule;
    }

    std::vector<std::string> conditions;
    DiagPolicyRule* rule;
};

TEST_F(DiagPolicyRuleTest, ConstructorInitializesFields_TC001) {
    EXPECT_EQ(rule->status, DiagInfoStatus::WARNING);
    EXPECT_EQ(rule->message, "Voltage too high");
    ASSERT_EQ(rule->conditions.size(), 2);
    EXPECT_EQ(rule->conditions[0], "cond_1");
    EXPECT_EQ(rule->conditions[1], "cond_2");
    EXPECT_EQ(rule->condition_operator, ConditionOperator::AND);
}

TEST_F(DiagPolicyRuleTest, AllDiagInfoStatuses_TC002) {
    DiagPolicyRule pass_rule(DiagInfoStatus::PASS, "Pass message", {"c1"}, ConditionOperator::AND);
    DiagPolicyRule fail_rule(DiagInfoStatus::FATAL, "Fail message", {"c2"}, ConditionOperator::OR);
    DiagPolicyRule warning_rule(DiagInfoStatus::WARNING, "Warning message", {"c3"}, ConditionOperator::AND);
    DiagPolicyRule error_rule(DiagInfoStatus::FATAL, "Error message", {"c4"}, ConditionOperator::OR);
    
    EXPECT_EQ(pass_rule.status, DiagInfoStatus::PASS);
    EXPECT_EQ(fail_rule.status, DiagInfoStatus::FATAL);
    EXPECT_EQ(warning_rule.status, DiagInfoStatus::WARNING);
    EXPECT_EQ(error_rule.status, DiagInfoStatus::FATAL);
}

TEST_F(DiagPolicyRuleTest, AllConditionOperators_TC003) {
    DiagPolicyRule and_rule(DiagInfoStatus::PASS, "AND message", {"c1", "c2"}, ConditionOperator::AND);
    DiagPolicyRule or_rule(DiagInfoStatus::PASS, "OR message", {"c3", "c4"}, ConditionOperator::OR);
    
    EXPECT_EQ(and_rule.condition_operator, ConditionOperator::AND);
    EXPECT_EQ(or_rule.condition_operator, ConditionOperator::OR);
}

TEST_F(DiagPolicyRuleTest, EmptyConditions_TC004) {
    std::vector<std::string> empty_conds;
    DiagPolicyRule empty_rule(DiagInfoStatus::PASS, "No conditions", empty_conds, ConditionOperator::AND);
    EXPECT_EQ(empty_rule.conditions.size(), 0);
}

TEST_F(DiagPolicyRuleTest, SingleCondition_TC005) {
    std::vector<std::string> single_cond = {"single"};
    DiagPolicyRule single_rule(DiagInfoStatus::FATAL, "Single condition", single_cond, ConditionOperator::OR);
    EXPECT_EQ(single_rule.conditions.size(), 1);
    EXPECT_EQ(single_rule.conditions[0], "single");
}

TEST_F(DiagPolicyRuleTest, MultipleConditions_TC006) {
    std::vector<std::string> multi_conds = {"c1", "c2", "c3", "c4", "c5"};
    DiagPolicyRule multi_rule(DiagInfoStatus::WARNING, "Multiple conditions", multi_conds, ConditionOperator::AND);
    EXPECT_EQ(multi_rule.conditions.size(), 5);
    for (size_t i = 0; i < multi_conds.size(); ++i) {
        EXPECT_EQ(multi_rule.conditions[i], multi_conds[i]);
    }
}

TEST_F(DiagPolicyRuleTest, EmptyMessage_TC007) {
    DiagPolicyRule empty_msg_rule(DiagInfoStatus::PASS, "", {"c1"}, ConditionOperator::AND);
    EXPECT_EQ(empty_msg_rule.message, "");
}

TEST_F(DiagPolicyRuleTest, LongMessage_TC008) {
    std::string long_msg(5000, 'x');
    DiagPolicyRule long_msg_rule(DiagInfoStatus::FATAL, long_msg, {"c1"}, ConditionOperator::OR);
    EXPECT_EQ(long_msg_rule.message, long_msg);
}

TEST_F(DiagPolicyRuleTest, SpecialCharactersInMessage_TC009) {
    std::string special_msg = "Error: voltage exceeded limit by 50%! @#$%^&*()";
    DiagPolicyRule special_rule(DiagInfoStatus::FATAL, special_msg, {"c1"}, ConditionOperator::AND);
    EXPECT_EQ(special_rule.message, special_msg);
}

TEST_F(DiagPolicyRuleTest, SpecialCharactersInConditions_TC010) {
    std::vector<std::string> special_conds = {"cond-1_test", "cond.2.test", "cond@3"};
    DiagPolicyRule special_cond_rule(DiagInfoStatus::PASS, "msg", special_conds, ConditionOperator::OR);
    EXPECT_EQ(special_cond_rule.conditions.size(), 3);
    EXPECT_EQ(special_cond_rule.conditions[0], "cond-1_test");
    EXPECT_EQ(special_cond_rule.conditions[1], "cond.2.test");
    EXPECT_EQ(special_cond_rule.conditions[2], "cond@3");
}

TEST_F(DiagPolicyRuleTest, LongConditionNames_TC011) {
    std::string long_cond(500, 'a');
    std::vector<std::string> long_conds = {long_cond};
    DiagPolicyRule long_cond_rule(DiagInfoStatus::WARNING, "msg", long_conds, ConditionOperator::AND);
    EXPECT_EQ(long_cond_rule.conditions[0], long_cond);
}

TEST_F(DiagPolicyRuleTest, MixedConditionsAndOperators_TC012) {
    std::vector<std::string> mixed_conds = {"temp_high", "voltage_low", "pressure_normal"};
    DiagPolicyRule and_mixed(DiagInfoStatus::FATAL, "Critical error", mixed_conds, ConditionOperator::AND);
    DiagPolicyRule or_mixed(DiagInfoStatus::WARNING, "Warning state", mixed_conds, ConditionOperator::OR);
    
    EXPECT_EQ(and_mixed.condition_operator, ConditionOperator::AND);
    EXPECT_EQ(or_mixed.condition_operator, ConditionOperator::OR);
    EXPECT_EQ(and_mixed.conditions.size(), 3);
    EXPECT_EQ(or_mixed.conditions.size(), 3);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
