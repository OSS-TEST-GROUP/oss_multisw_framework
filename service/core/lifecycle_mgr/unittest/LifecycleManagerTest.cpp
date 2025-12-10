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
        Mock_Service = new MockService();
        Mock_StateManager = new MockStateManager(mgr);
        Mock_ServiceRegistry = new MockServiceRegistry();
    }
    virtual void TearDown(){
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete Mock_Service;
        Mock_Service = nullptr;
        delete Mock_StateManager;
        Mock_StateManager = nullptr;
        delete Mock_ServiceRegistry;
        Mock_ServiceRegistry = nullptr;
    }

    TestableLifecycleManager mgr;
};

TEST_F(LifecycleManagerTest, Start_AllDependenciesSucceed_ReturnsTrue) {
    auto state_manager = std::make_shared<MockStateManager>(mgr);
    auto policy_manager = std::make_shared<MockPolicyManager>();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    auto state_monitor = std::make_shared<MockStateMonitor>(mgr);

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



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
