// Unit tests for PolicyManager class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "policy/PolicyManager.hpp"
#include "policy/PolicyAlways.hpp"

#include "mock/mock_service.hpp"
#include "mock/mock_logger.hpp"

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
    virtual void SetUp(){
        Mock_Logger = new MockLogger();
        Mock_Service = new MockService();
    }
    virtual void TearDown(){
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete Mock_Service;
        Mock_Service = nullptr;
    }
};


TEST_F(PolicyManagerTest, StartAndStop) {
    PolicyManager manager;
    EXPECT_TRUE(manager.Start());
    manager.Stop();

}

// Add more tests for PolicyManager as needed
TEST_F(PolicyManagerTest, GetPolicyByEnumReturnsCorrectPolicy) {
    PolicyManager manager;
    EXPECT_TRUE(manager.Start());

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

TEST_F(PolicyManagerTest, GetPolicyByNameReturnsCorrectPolicy) {
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

TEST_F(PolicyManagerTest, EvaluateReturnsPolicyAction) {
    PolicyManager manager;
    manager.Start();

    auto mock_policy = std::make_shared<PolicyAlways>(RecoveryPolicy::ALWAYS, "Policy-Always");
    manager.policies_[RecoveryPolicy::ALWAYS] = mock_policy;

    auto service = std::make_shared<MockService>();
    EXPECT_CALL(*service, GetRecoveryPolicy()).WillOnce(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*service, GetServiceMode()).WillOnce(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*service, GetServiceState()).WillOnce(Return(ServiceState::NOT_RUNNING));

    // PolicyAlways::Evaluate returns ServiceControlAction::RESTART by default
    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::START);
}

TEST_F(PolicyManagerTest, EvaluateReturnsNoneIfPolicyNotFound) {
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
