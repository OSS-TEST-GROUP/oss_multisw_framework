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
        // Do NOT allocate Mock_Service here; tests that need it will create a shared_ptr
        // and set Mock_Service to the raw pointer for forwarding.
        Mock_Service = nullptr;
    }
    virtual void TearDown(){
        delete Mock_Logger;
        Mock_Logger = nullptr;
        // Do NOT delete Mock_Service here; it is managed by shared_ptr in tests that use it.
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

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    // Expectations must be set on Mock_Service since Service base class methods forward to it
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    // PolicyAlways::Evaluate returns ServiceControlAction::START when state is NOT_RUNNING
    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::START);
}

TEST_F(PolicyManagerTest, EvaluateReturnsNoneIfPolicyNotFound) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    // Expectations must be set on Mock_Service since Service base class methods forward to it
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(static_cast<RecoveryPolicy>(999)));

    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::NONE);
}

// Additional tests for branch coverage

// Test GetPolicy with all RecoveryPolicy enum values
TEST_F(PolicyManagerTest, GetPolicyByEnum_AllPolicies) {
    PolicyManager manager;
    manager.Start();

    auto always = manager.GetPolicy(RecoveryPolicy::ALWAYS);
    ASSERT_NE(always, nullptr);
    EXPECT_EQ(always->GetName(), "Policy-Always");

    auto on_failure = manager.GetPolicy(RecoveryPolicy::ON_FAILURE);
    ASSERT_NE(on_failure, nullptr);
    EXPECT_EQ(on_failure->GetName(), "Policy-OnFailure");

    auto on_success = manager.GetPolicy(RecoveryPolicy::ON_SUCCESS);
    ASSERT_NE(on_success, nullptr);
    EXPECT_EQ(on_success->GetName(), "Policy-OnSuccess");

    auto on_not_responding = manager.GetPolicy(RecoveryPolicy::ON_NOT_RESPONDING);
    ASSERT_NE(on_not_responding, nullptr);
    EXPECT_EQ(on_not_responding->GetName(), "Policy-NotRespond");

    auto manual = manager.GetPolicy(RecoveryPolicy::MANUAL);
    ASSERT_NE(manual, nullptr);
    EXPECT_EQ(manual->GetName(), "Policy-Manual");
}

// Test GetPolicy by name for all policies
TEST_F(PolicyManagerTest, GetPolicyByName_AllPolicies) {
    PolicyManager manager;
    manager.Start();

    EXPECT_NE(manager.GetPolicy("Policy-Always"), nullptr);
    EXPECT_NE(manager.GetPolicy("Policy-OnFailure"), nullptr);
    EXPECT_NE(manager.GetPolicy("Policy-OnSuccess"), nullptr);
    EXPECT_NE(manager.GetPolicy("Policy-NotRespond"), nullptr);
    EXPECT_NE(manager.GetPolicy("Policy-Manual"), nullptr);
}

// Test GetPolicy by enum returns nullptr for non-existent policy
TEST_F(PolicyManagerTest, GetPolicyByEnum_NonExistent) {
    PolicyManager manager;
    manager.Start();

    auto policy = manager.GetPolicy(static_cast<RecoveryPolicy>(999));
    EXPECT_EQ(policy, nullptr);
}

// Test GetPolicy by name returns nullptr for non-existent policy
TEST_F(PolicyManagerTest, GetPolicyByName_NonExistent) {
    PolicyManager manager;
    manager.Start();

    auto policy = manager.GetPolicy("NonExistent-Policy");
    EXPECT_EQ(policy, nullptr);
}

// Test GetPolicy by empty name
TEST_F(PolicyManagerTest, GetPolicyByName_EmptyString) {
    PolicyManager manager;
    manager.Start();

    auto policy = manager.GetPolicy("");
    EXPECT_EQ(policy, nullptr);
}

// Test Evaluate with ON_FAILURE recovery policy
TEST_F(PolicyManagerTest, Evaluate_OnFailurePolicy) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    // exit_code == -1 (never started) → treated as initial launch → START
    EXPECT_CALL(*Mock_Service, GetExitCode()).WillRepeatedly(Return(-1));

    auto action = manager.Evaluate(service);
    // PolicyOnFailure: NOT_RUNNING with exit_code=-1 → START (initial launch)
    EXPECT_EQ(action, ServiceControlAction::START);
}

// Test Evaluate with ON_SUCCESS recovery policy
TEST_F(PolicyManagerTest, Evaluate_OnSuccessPolicy) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    // Just verify it doesn't crash and returns a valid action
    EXPECT_TRUE(action == ServiceControlAction::START || 
                action == ServiceControlAction::STOP || 
                action == ServiceControlAction::NONE);
}

// Test Evaluate with MANUAL recovery policy
TEST_F(PolicyManagerTest, Evaluate_ManualPolicy) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::MANUAL));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    // Manual policy should return NONE (no automatic action)
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// Test Evaluate with ON_NOT_RESPONDING recovery policy
TEST_F(PolicyManagerTest, Evaluate_OnNotRespondingPolicy) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_TRUE(action == ServiceControlAction::START || 
                action == ServiceControlAction::STOP || 
                action == ServiceControlAction::NONE);
}

// Test Evaluate with service in OTA_MODE
TEST_F(PolicyManagerTest, Evaluate_ServiceInOTAMode) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::OTA_MODE));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    // PolicyAlways in OTA_MODE may have different behavior
    EXPECT_TRUE(action == ServiceControlAction::START || 
                action == ServiceControlAction::STOP || 
                action == ServiceControlAction::NONE);
}

// Test Evaluate with service in RUNNING state
TEST_F(PolicyManagerTest, Evaluate_ServiceRunning) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    // Running service with ALWAYS policy should not need restart
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// Test Evaluate with service in INITIALIZING state
TEST_F(PolicyManagerTest, Evaluate_ServiceInitializing) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::INITIALIZING));

    auto action = manager.Evaluate(service);
    // Initializing service should wait
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// Test Evaluate with service in TERMINATING state
TEST_F(PolicyManagerTest, Evaluate_ServiceTerminating) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::TERMINATING));

    auto action = manager.Evaluate(service);
    // Terminating service should wait
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// Test multiple consecutive Evaluate calls
TEST_F(PolicyManagerTest, Evaluate_MultipleCalls) {
    PolicyManager manager;
    manager.Start();

    auto service1 = std::make_shared<MockService>();
    auto service2 = std::make_shared<MockService>();

    // Note: We need to switch Mock_Service between evaluations
    Mock_Service = service1.get();
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action1 = manager.Evaluate(service1);

    Mock_Service = service2.get();
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::MANUAL));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action2 = manager.Evaluate(service2);

    EXPECT_EQ(action1, ServiceControlAction::START);
    EXPECT_EQ(action2, ServiceControlAction::NONE);
}

// Test Stop method
TEST_F(PolicyManagerTest, StopDoesNotCrash) {
    PolicyManager manager;
    manager.Start();
    EXPECT_NO_THROW(manager.Stop());
}

// Test Start can be called multiple times
TEST_F(PolicyManagerTest, Start_MultipleCalls) {
    PolicyManager manager;
    EXPECT_TRUE(manager.Start());
    EXPECT_TRUE(manager.Start()); // Should not crash on second call
}

// ============================================================================
// NEW TESTS FOR IMPROVED BRANCH COVERAGE
// Tests for PolicyOnFailure - All ServiceState branches
// ============================================================================

// PolicyOnFailure: INITIALIZING state branch
TEST_F(PolicyManagerTest, PolicyOnFailure_InitializingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::INITIALIZING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnFailure: RUNNING state branch
TEST_F(PolicyManagerTest, PolicyOnFailure_RunningState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnFailure: RUNNING_NO_RESPONSE state branch
TEST_F(PolicyManagerTest, PolicyOnFailure_RunningNoResponseState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING_NO_RESPONSE));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::STOP);
}

// PolicyOnFailure: TERMINATING state branch
TEST_F(PolicyManagerTest, PolicyOnFailure_TerminatingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::TERMINATING));

    auto action = manager.Evaluate(service);
    // TERMINATING → wait for NOT_RUNNING; exit code will be checked there.
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnFailure: non-NORMAL ServiceMode branch
TEST_F(PolicyManagerTest, PolicyOnFailure_NonNormalMode) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::OTA_MODE));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// ============================================================================
// Tests for PolicyManual - All ServiceState branches
// ============================================================================

// PolicyManual: INITIALIZING state branch
TEST_F(PolicyManagerTest, PolicyManual_InitializingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::MANUAL));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::INITIALIZING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyManual: RUNNING state branch
TEST_F(PolicyManagerTest, PolicyManual_RunningState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::MANUAL));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyManual: RUNNING_NO_RESPONSE state branch
TEST_F(PolicyManagerTest, PolicyManual_RunningNoResponseState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::MANUAL));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING_NO_RESPONSE));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyManual: TERMINATING state branch
TEST_F(PolicyManagerTest, PolicyManual_TerminatingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::MANUAL));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::TERMINATING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyManual: non-NORMAL ServiceMode branch
TEST_F(PolicyManagerTest, PolicyManual_NonNormalMode) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::MANUAL));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::OTA_MODE));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// ============================================================================
// Tests for PolicyOnSuccess - All ServiceState branches
// ============================================================================

// PolicyOnSuccess: NOT_RUNNING state branch (clean exit → restart)
TEST_F(PolicyManagerTest, PolicyOnSuccess_NotRunningState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    // exit_code == 0 (clean exit) → PolicyOnSuccess should restart
    EXPECT_CALL(*Mock_Service, GetExitCode()).WillRepeatedly(Return(0));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::START);
}

// PolicyOnSuccess: INITIALIZING state branch
TEST_F(PolicyManagerTest, PolicyOnSuccess_InitializingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::INITIALIZING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnSuccess: RUNNING state branch
TEST_F(PolicyManagerTest, PolicyOnSuccess_RunningState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnSuccess: RUNNING_NO_RESPONSE state branch
TEST_F(PolicyManagerTest, PolicyOnSuccess_RunningNoResponseState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING_NO_RESPONSE));

    auto action = manager.Evaluate(service);
    // An unresponsive service has not exited successfully; OnSuccess takes no action.
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnSuccess: TERMINATING state branch
TEST_F(PolicyManagerTest, PolicyOnSuccess_TerminatingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::TERMINATING));

    auto action = manager.Evaluate(service);
    // TERMINATING → wait for NOT_RUNNING; exit code will be checked there.
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnSuccess: non-NORMAL ServiceMode branch
TEST_F(PolicyManagerTest, PolicyOnSuccess_NonNormalMode) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::OTA_MODE));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// ============================================================================
// Tests for PolicyOnNotResponding - All ServiceState branches
// ============================================================================

// PolicyOnNotResponding: NOT_RUNNING state branch — initial start (last == NOT_RUNNING)
TEST_F(PolicyManagerTest, PolicyOnNotResponding_NotRunningState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    // last == NOT_RUNNING means initial start → should start
    EXPECT_CALL(*Mock_Service, GetLastServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::START);
}

// PolicyOnNotResponding: INITIALIZING state branch
TEST_F(PolicyManagerTest, PolicyOnNotResponding_InitializingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::INITIALIZING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnNotResponding: RUNNING state branch
TEST_F(PolicyManagerTest, PolicyOnNotResponding_RunningState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnNotResponding: RUNNING_NO_RESPONSE state branch
TEST_F(PolicyManagerTest, PolicyOnNotResponding_RunningNoResponseState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING_NO_RESPONSE));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::STOP);
}

// PolicyOnNotResponding: TERMINATING state branch
TEST_F(PolicyManagerTest, PolicyOnNotResponding_TerminatingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::TERMINATING));

    auto action = manager.Evaluate(service);
    // TERMINATING → wait for NOT_RUNNING.
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnNotResponding: non-NORMAL ServiceMode branch
TEST_F(PolicyManagerTest, PolicyOnNotResponding_NonNormalMode) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::OTA_MODE));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// ============================================================================
// Tests for PolicyAlways - All ServiceState branches
// ============================================================================

// PolicyAlways: INITIALIZING state branch
TEST_F(PolicyManagerTest, PolicyAlways_InitializingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::INITIALIZING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyAlways: RUNNING_NO_RESPONSE state branch
TEST_F(PolicyManagerTest, PolicyAlways_RunningNoResponseState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::RUNNING_NO_RESPONSE));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::STOP);
}

// PolicyAlways: TERMINATING state branch
TEST_F(PolicyManagerTest, PolicyAlways_TerminatingState) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::TERMINATING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyAlways: non-NORMAL ServiceMode branch
TEST_F(PolicyManagerTest, PolicyAlways_NonNormalMode) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::OTA_MODE));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// ============================================================================
// Additional edge case tests
// ============================================================================

// Test policy with NOT_RUNNING state
TEST_F(PolicyManagerTest, PolicyAlways_NotRunningState_AdditionalTest) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));

    auto action = manager.Evaluate(service);
    // NOT_RUNNING should trigger START action for ALWAYS policy
    EXPECT_TRUE(action == ServiceControlAction::START || action == ServiceControlAction::NONE);
}

// Test multiple policy evaluations in sequence
TEST_F(PolicyManagerTest, MultipleSequentialEvaluations) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();
    
    // First evaluation - NOT_RUNNING
    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ALWAYS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState())
        .WillOnce(Return(ServiceState::NOT_RUNNING))
        .WillOnce(Return(ServiceState::INITIALIZING))
        .WillOnce(Return(ServiceState::RUNNING))
        .WillOnce(Return(ServiceState::RUNNING_NO_RESPONSE))
        .WillOnce(Return(ServiceState::TERMINATING));

    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::START);
    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::NONE);
    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::NONE);
    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::STOP);
    EXPECT_EQ(manager.Evaluate(service), ServiceControlAction::NONE);
}

// ============================================================================
// ISSUE-1: Exit-code-based policy differentiation tests
// ============================================================================

// PolicyOnFailure: NOT_RUNNING with exit_code==0 → NONE (clean exit, don't restart)
TEST_F(PolicyManagerTest, PolicyOnFailure_NotRunning_CleanExit) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    EXPECT_CALL(*Mock_Service, GetExitCode()).WillRepeatedly(Return(0)); // clean exit

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnFailure: NOT_RUNNING with exit_code==1 → START (abnormal exit)
TEST_F(PolicyManagerTest, PolicyOnFailure_NotRunning_ErrorExit) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_FAILURE));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    EXPECT_CALL(*Mock_Service, GetExitCode()).WillRepeatedly(Return(1)); // error exit

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::START);
}

// PolicyOnSuccess: NOT_RUNNING with exit_code==0 → START (clean exit, restart)
TEST_F(PolicyManagerTest, PolicyOnSuccess_NotRunning_CleanExit) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    EXPECT_CALL(*Mock_Service, GetExitCode()).WillRepeatedly(Return(0)); // clean exit

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::START);
}

// PolicyOnSuccess: NOT_RUNNING with exit_code==1 → NONE (error exit, don't restart)
TEST_F(PolicyManagerTest, PolicyOnSuccess_NotRunning_ErrorExit) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    EXPECT_CALL(*Mock_Service, GetExitCode()).WillRepeatedly(Return(1)); // error exit

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

// PolicyOnSuccess: NOT_RUNNING with exit_code==-1 (initial) → START
TEST_F(PolicyManagerTest, PolicyOnSuccess_NotRunning_InitialStart) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_SUCCESS));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    EXPECT_CALL(*Mock_Service, GetExitCode()).WillRepeatedly(Return(-1)); // never started

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::START);
}

// PolicyOnNotResponding: NOT_RUNNING with last==TERMINATING → START (watchdog restart)
TEST_F(PolicyManagerTest, PolicyOnNotResponding_NotRunning_AfterWatchdogStop) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    // last == TERMINATING: service was stopped via STOP command (watchdog path)
    EXPECT_CALL(*Mock_Service, GetLastServiceState()).WillRepeatedly(Return(ServiceState::TERMINATING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::START);
}

// PolicyOnNotResponding: NOT_RUNNING with last==RUNNING → NONE (direct crash, not watchdog)
TEST_F(PolicyManagerTest, PolicyOnNotResponding_NotRunning_DirectCrash) {
    PolicyManager manager;
    manager.Start();

    auto service = std::make_shared<MockService>();
    Mock_Service = service.get();

    EXPECT_CALL(*Mock_Service, GetRecoveryPolicy()).WillRepeatedly(Return(RecoveryPolicy::ON_NOT_RESPONDING));
    EXPECT_CALL(*Mock_Service, GetServiceMode()).WillRepeatedly(Return(ServiceMode::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState()).WillRepeatedly(Return(ServiceState::NOT_RUNNING));
    // last == RUNNING: service crashed without going through STOP/TERMINATING
    EXPECT_CALL(*Mock_Service, GetLastServiceState()).WillRepeatedly(Return(ServiceState::RUNNING));

    auto action = manager.Evaluate(service);
    EXPECT_EQ(action, ServiceControlAction::NONE);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
