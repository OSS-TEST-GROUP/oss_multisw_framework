#include <gtest/gtest.h>

#include "diagnostic_analyzer/DiagPolicyRule.cpp"

using namespace diagnostic_manager;

class DiagPolicyRuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        conditions = {"cond_1", "cond_2"};
        rule = new DiagPolicyRule(DiagInfoStatus::WARNING, "Voltage too high", conditions);
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
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
