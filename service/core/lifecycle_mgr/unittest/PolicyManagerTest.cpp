// Unit tests for PolicyManager class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unittest/mock/mock_policy_manager.hpp"
#include "unittest/mock/mock_service.hpp"
#include "unittest/mock/mock_policy_always.hpp"
#include "unittest/mock/mock_policy_base.hpp"

using namespace lifecycle_manager;

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

class PolicyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST(PolicyManagerTest, StartAndStop) {
    MockPolicyManager policy;
    EXPECT_CALL(policy, Start()).WillOnce(::testing::Return(true));
    EXPECT_TRUE(policy.Start());
    EXPECT_CALL(policy, Stop()).Times(1);
    policy.Stop();
}

// Add more tests for PolicyManager as needed
TEST(PolicyManagerTest, GetPolicyByEnumReturnsCorrectPolicy) {
    PolicyManager manager;
    manager.Start();

    auto policy = manager.GetPolicy(RecoveryPolicy::ALWAYS);
    ASSERT_NE(policy, nullptr);
    EXPECT_EQ(policy->GetName(), "Policy-Always");

    policy = manager.GetPolicy(RecoveryPolicy::ON_FAILURE);
    ASSERT_NE(policy, nullptr);
    EXPECT_EQ(policy->GetName(), "Policy-OnFailure");

    policy = manager.GetPolicy(RecoveryPolicy::MANUAL);
    ASSERT_NE(policy, nullptr);
    EXPECT_EQ(policy->GetName(), "Policy-Manual");
}

TEST(PolicyManagerTest, GetPolicyByNameReturnsCorrectPolicy) {
    PolicyManager manager;
    manager.Start();

    auto policy = manager.GetPolicy("Policy-OnSuccess");
    ASSERT_NE(policy, nullptr);
    EXPECT_EQ(policy->GetName(), "Policy-OnSuccess");

    policy = manager.GetPolicy("Policy-NotRespond");
    ASSERT_NE(policy, nullptr);
    EXPECT_EQ(policy->GetName(), "Policy-NotRespond");

    policy = manager.GetPolicy("NonExistentPolicy");
    EXPECT_EQ(policy, nullptr);
}

TEST(PolicyManagerTest, EvaluateReturnsPolicyAction) {
    PolicyManager manager;
    manager.Start();

    auto mock_policy = std::make_shared<PolicyAlways>(RecoveryPolicy::ALWAYS, "Policy-Always");
    manager.policies_[RecoveryPolicy::ALWAYS] = mock_policy;

    auto service = std::make_shared<MockService>();
    EXPECT_CALL(*service, GetRecoveryPolicy()).WillOnce(Return(RecoveryPolicy::ALWAYS));

    // PolicyAlways::Evaluate returns ServiceControlAction::RESTART by default
    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::START);
}

TEST(PolicyManagerTest, EvaluateReturnsNoneIfPolicyNotFound) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    EXPECT_CALL(*service, GetRecoveryPolicy()).WillOnce(Return(static_cast<RecoveryPolicy>(999)));

    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::NONE);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
