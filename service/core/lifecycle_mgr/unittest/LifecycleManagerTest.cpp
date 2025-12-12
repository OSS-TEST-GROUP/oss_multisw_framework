// Unit tests for LifecycleManager using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "manager/LifecycleManager.hpp"

#include "mock/mock_state_manager.hpp"
#include "mock/mock_policy_manager.hpp"
#include "mock/mock_mw_service_manager.hpp"
#include "mock/mock_state_monitor.hpp"
#include "mock/mock_service_registry.hpp"
#include "mock/mock_manager_base.hpp"
#include "mock/mock_service.hpp"
#include "mock/mock_endpoint_factory.hpp"
#include "mock/mock_dds_transmitter.hpp"
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

// Helper test fixture to access private members via friend declaration
class LifecycleManagerTest : public ::testing::Test {
protected:
    class TestableLifecycleManager : public LifecycleManager {
    public:
        // Provide setters for test injection
        void SetStateManager(std::shared_ptr<StateManager> sm) { state_manager_ = sm; }
        void SetPolicyManager(std::shared_ptr<PolicyManager> pm) { policy_manager_ = pm; }
        void SetMWServiceManager(std::shared_ptr<MWServiceManager> mw) { mw_service_manager_ = mw; }
        void SetStateMonitor(std::shared_ptr<StateMonitor> sm) { state_monitor_ = sm; }
        void SetServiceRegistry(std::shared_ptr<ServiceRegistry> sr) { service_registry_ = sr; }
    };

    virtual void SetUp(){
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        // Reset all global pointers
        Mock_StateManager = nullptr;
        Mock_PolicyManager = nullptr;
        Mock_MWServiceManager = nullptr;
        Mock_StateMonitor = nullptr;
        Mock_ServiceRegistry = nullptr;
        Mock_Service = nullptr;
        
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    TestableLifecycleManager mgr;
};

TEST_F(LifecycleManagerTest, Start_AllDependenciesSucceed_ReturnsTrue) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

    // Set global pointers for forwarding
    Mock_StateManager = state_manager.get();
    Mock_PolicyManager = policy_manager.get();
    Mock_MWServiceManager = mw_service_manager.get();
    Mock_StateMonitor = state_monitor.get();

    EXPECT_CALL(*state_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*policy_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*mw_service_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*state_monitor, Start()).WillOnce(Return(true));

    mgr.SetStateManager(state_manager);
    mgr.SetPolicyManager(policy_manager);
    mgr.SetMWServiceManager(mw_service_manager);
    mgr.SetStateMonitor(state_monitor);

    EXPECT_TRUE(mgr.Start());
}

TEST_F(LifecycleManagerTest, Start_DependencyFails_ReturnsFalse) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

    // Set global pointers for forwarding
    Mock_StateManager = state_manager.get();
    Mock_PolicyManager = policy_manager.get();
    Mock_MWServiceManager = mw_service_manager.get();
    Mock_StateMonitor = state_monitor.get();

    EXPECT_CALL(*state_manager, Start()).WillOnce(Return(false));
    EXPECT_CALL(*policy_manager, Start()).Times(0);
    EXPECT_CALL(*mw_service_manager, Start()).Times(0);
    EXPECT_CALL(*state_monitor, Start()).Times(0);

    mgr.SetStateManager(state_manager);
    mgr.SetPolicyManager(policy_manager);
    mgr.SetMWServiceManager(mw_service_manager);
    mgr.SetStateMonitor(state_monitor);

    EXPECT_FALSE(mgr.Start());
}

TEST_F(LifecycleManagerTest, GetService_RegistryReturnsService_ReturnsService) {
    auto registry = std::make_shared<MockServiceRegistry>();
    Mock_ServiceRegistry = registry.get();  // Set global pointer for forwarding
    
    auto expected_service = std::make_shared<lifecycle_manager::Service>();
    EXPECT_CALL(*registry, GetService("test")).WillOnce(Return(expected_service));
    mgr.SetServiceRegistry(registry);
    auto result = mgr.GetService("test");
    EXPECT_EQ(result, expected_service);
}

TEST_F(LifecycleManagerTest, GetService_RegistryNull_ReturnsNullptr) {
    mgr.SetServiceRegistry(nullptr);
    auto result = mgr.GetService("test");
    EXPECT_EQ(result, nullptr);
}

// Additional tests for branch coverage

// Test Stop method
TEST_F(LifecycleManagerTest, Stop_SetsTerminateState) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

    // Set global pointers for forwarding
    Mock_StateManager = state_manager.get();
    Mock_PolicyManager = policy_manager.get();
    Mock_MWServiceManager = mw_service_manager.get();
    Mock_StateMonitor = state_monitor.get();

    EXPECT_CALL(*state_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*policy_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*mw_service_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*state_monitor, Start()).WillOnce(Return(true));
    EXPECT_CALL(*state_manager, RequestStopAll()).Times(1);

    mgr.SetStateManager(state_manager);
    mgr.SetPolicyManager(policy_manager);
    mgr.SetMWServiceManager(mw_service_manager);
    mgr.SetStateMonitor(state_monitor);

    EXPECT_TRUE(mgr.Start());
    EXPECT_NO_THROW(mgr.Stop());
}

// Test PolicyManager failure
TEST_F(LifecycleManagerTest, Start_PolicyManagerFails_ReturnsFalse) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

    // Set global pointers for forwarding
    Mock_StateManager = state_manager.get();
    Mock_PolicyManager = policy_manager.get();
    Mock_MWServiceManager = mw_service_manager.get();
    Mock_StateMonitor = state_monitor.get();

    EXPECT_CALL(*state_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*policy_manager, Start()).WillOnce(Return(false));
    EXPECT_CALL(*mw_service_manager, Start()).Times(0);
    EXPECT_CALL(*state_monitor, Start()).Times(0);

    mgr.SetStateManager(state_manager);
    mgr.SetPolicyManager(policy_manager);
    mgr.SetMWServiceManager(mw_service_manager);
    mgr.SetStateMonitor(state_monitor);

    EXPECT_FALSE(mgr.Start());
}

// Test MWServiceManager failure
TEST_F(LifecycleManagerTest, Start_MWServiceManagerFails_ReturnsFalse) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

    // Set global pointers for forwarding
    Mock_StateManager = state_manager.get();
    Mock_PolicyManager = policy_manager.get();
    Mock_MWServiceManager = mw_service_manager.get();
    Mock_StateMonitor = state_monitor.get();

    EXPECT_CALL(*state_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*policy_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*mw_service_manager, Start()).WillOnce(Return(false));
    EXPECT_CALL(*state_monitor, Start()).Times(0);

    mgr.SetStateManager(state_manager);
    mgr.SetPolicyManager(policy_manager);
    mgr.SetMWServiceManager(mw_service_manager);
    mgr.SetStateMonitor(state_monitor);

    EXPECT_FALSE(mgr.Start());
}

// Test StateMonitor failure
TEST_F(LifecycleManagerTest, Start_StateMonitorFails_ReturnsFalse) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

    // Set global pointers for forwarding
    Mock_StateManager = state_manager.get();
    Mock_PolicyManager = policy_manager.get();
    Mock_MWServiceManager = mw_service_manager.get();
    Mock_StateMonitor = state_monitor.get();

    EXPECT_CALL(*state_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*policy_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*mw_service_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*state_monitor, Start()).WillOnce(Return(false));

    mgr.SetStateManager(state_manager);
    mgr.SetPolicyManager(policy_manager);
    mgr.SetMWServiceManager(mw_service_manager);
    mgr.SetStateMonitor(state_monitor);

    EXPECT_FALSE(mgr.Start());
}

// Test GetTransmitter returns nullptr initially
TEST_F(LifecycleManagerTest, GetTransmitter_NotInitialized_ReturnsNullptr) {
    auto result = mgr.GetTransmitter();
    EXPECT_EQ(result, nullptr);
}

// Test GetStateMgr returns nullptr initially
TEST_F(LifecycleManagerTest, GetStateMgr_NotInitialized_ReturnsNullptr) {
    auto result = mgr.GetStateMgr();
    EXPECT_EQ(result, nullptr);
}

// Test GetPolicy returns nullptr initially
TEST_F(LifecycleManagerTest, GetPolicy_NotInitialized_ReturnsNullptr) {
    auto result = mgr.GetPolicy();
    EXPECT_EQ(result, nullptr);
}

// Test GetMWService returns nullptr initially
TEST_F(LifecycleManagerTest, GetMWService_NotInitialized_ReturnsNullptr) {
    auto result = mgr.GetMWService();
    EXPECT_EQ(result, nullptr);
}

// Test GetMonitor returns nullptr initially
TEST_F(LifecycleManagerTest, GetMonitor_NotInitialized_ReturnsNullptr) {
    auto result = mgr.GetMonitor();
    EXPECT_EQ(result, nullptr);
}

// Test GetRegistry returns nullptr initially
TEST_F(LifecycleManagerTest, GetRegistry_NotInitialized_ReturnsNullptr) {
    auto result = mgr.GetRegistry();
    EXPECT_EQ(result, nullptr);
}

// Test GetEndpointFactory returns nullptr initially
TEST_F(LifecycleManagerTest, GetEndpointFactory_NotInitialized_ReturnsNullptr) {
    auto result = mgr.GetEndpointFactory();
    EXPECT_EQ(result, nullptr);
}

// Test GetService returns correct service
TEST_F(LifecycleManagerTest, GetService_ReturnsCorrectService) {
    auto registry = std::make_shared<MockServiceRegistry>();
    Mock_ServiceRegistry = registry.get();  // Set global pointer for forwarding
    
    // Use MockService instead of Service since Service::GetName() forwards to Mock_Service
    auto expected_service = std::make_shared<MockService>("test_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::OsType::ROS2);
    Mock_Service = expected_service.get();  // Set global pointer for forwarding
    
    ON_CALL(*expected_service, GetName()).WillByDefault(Return("test_svc"));
    EXPECT_CALL(*registry, GetService("test_svc")).WillOnce(Return(expected_service));
    
    mgr.SetServiceRegistry(registry);
    auto result = mgr.GetService("test_svc");
    
    EXPECT_EQ(result, expected_service);
    EXPECT_EQ(result->GetName(), "test_svc");
}

// Test GetService returns nullptr for non-existent service
TEST_F(LifecycleManagerTest, GetService_NonExistent_ReturnsNullptr) {
    auto registry = std::make_shared<MockServiceRegistry>();
    Mock_ServiceRegistry = registry.get();  // Set global pointer for forwarding
    
    EXPECT_CALL(*registry, GetService("nonexistent")).WillOnce(Return(nullptr));
    
    mgr.SetServiceRegistry(registry);
    auto result = mgr.GetService("nonexistent");
    
    EXPECT_EQ(result, nullptr);
}

// Test Stop without Start (should not crash)
TEST_F(LifecycleManagerTest, Stop_WithoutStart_DoesNotCrash) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    Mock_StateManager = state_manager.get();  // Set global pointer for forwarding
    
    mgr.SetStateManager(state_manager);
    
    // No RequestStopAll expected since manager is not RUNNING
    EXPECT_NO_THROW(mgr.Stop());
}

// Test multiple Start calls
TEST_F(LifecycleManagerTest, Start_MultipleCalls) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

    // Set global pointers for forwarding
    Mock_StateManager = state_manager.get();
    Mock_PolicyManager = policy_manager.get();
    Mock_MWServiceManager = mw_service_manager.get();
    Mock_StateMonitor = state_monitor.get();

    EXPECT_CALL(*state_manager, Start()).WillRepeatedly(Return(true));
    EXPECT_CALL(*policy_manager, Start()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mw_service_manager, Start()).WillRepeatedly(Return(true));
    EXPECT_CALL(*state_monitor, Start()).WillRepeatedly(Return(true));

    mgr.SetStateManager(state_manager);
    mgr.SetPolicyManager(policy_manager);
    mgr.SetMWServiceManager(mw_service_manager);
    mgr.SetStateMonitor(state_monitor);

    EXPECT_TRUE(mgr.Start());
    EXPECT_TRUE(mgr.Start()); // Second call should also succeed
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
