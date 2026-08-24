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
    auto expected_service = std::make_shared<MockService>("test_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::ROS2);
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

// ============================================================================
// Initialize Method Coverage Tests
// ============================================================================

TEST_F(LifecycleManagerTest, Initialize_Success_ReturnsTrue) {
    // Create mock objects
    auto dds_transmitter = std::make_shared<MockDDSTransmitter>();
    auto endpoint_factory = new MockEndPointFactory();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    
    // Set global pointers for forwarding
    Mock_DDSTransmitter = dds_transmitter.get();
    Mock_EndPointFactory = endpoint_factory;
    Mock_MWServiceManager = mw_service_manager.get();
    
    // Allow leak for endpoint_factory since it's managed by LifecycleManager after Initialize
    testing::Mock::AllowLeak(endpoint_factory);
    
    // Set expectations for Initialize
    EXPECT_CALL(*mw_service_manager, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*dds_transmitter, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*endpoint_factory, Initialize()).WillOnce(Return(0));
    EXPECT_CALL(*endpoint_factory, CreateEndPoints(_, _)).WillOnce(Return(0));
    
    // Call Initialize on a fresh LifecycleManager
    TestableLifecycleManager fresh_mgr;
    bool result = fresh_mgr.Initialize();
    
    EXPECT_TRUE(result);
    
    // Verify that components were created
    EXPECT_NE(nullptr, fresh_mgr.GetStateMgr());
    EXPECT_NE(nullptr, fresh_mgr.GetPolicy());
    EXPECT_NE(nullptr, fresh_mgr.GetMWService());
    EXPECT_NE(nullptr, fresh_mgr.GetMonitor());
    EXPECT_NE(nullptr, fresh_mgr.GetRegistry());
    EXPECT_NE(nullptr, fresh_mgr.GetTransmitter());
    EXPECT_NE(nullptr, fresh_mgr.GetEndpointFactory());
    
    // Clean up
    Mock_DDSTransmitter = nullptr;
    Mock_EndPointFactory = nullptr;
    Mock_MWServiceManager = nullptr;
}

TEST_F(LifecycleManagerTest, Initialize_CreatesAllComponents) {
    // Create mock objects
    auto dds_transmitter = std::make_shared<MockDDSTransmitter>();
    auto endpoint_factory = new MockEndPointFactory();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    
    // Set global pointers for forwarding
    Mock_DDSTransmitter = dds_transmitter.get();
    Mock_EndPointFactory = endpoint_factory;
    Mock_MWServiceManager = mw_service_manager.get();
    
    // Allow leak for endpoint_factory since it's managed by LifecycleManager after Initialize
    testing::Mock::AllowLeak(endpoint_factory);
    
    // Set expectations - Initialize always returns true even if components fail internally
    EXPECT_CALL(*mw_service_manager, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*dds_transmitter, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*endpoint_factory, Initialize()).WillOnce(Return(0));
    EXPECT_CALL(*endpoint_factory, CreateEndPoints(_, _)).WillOnce(Return(0));
    
    TestableLifecycleManager fresh_mgr;
    EXPECT_TRUE(fresh_mgr.Initialize());
    
    // Verify state is set to MANAGER_INIT
    // Note: We can't directly check state, but we can verify components exist
    EXPECT_NE(nullptr, fresh_mgr.GetRegistry());
    
    // Clean up
    Mock_DDSTransmitter = nullptr;
    Mock_EndPointFactory = nullptr;
    Mock_MWServiceManager = nullptr;
}

TEST_F(LifecycleManagerTest, Initialize_ThenStart_FullLifecycle) {
    // Create mock objects for Initialize
    auto dds_transmitter = std::make_shared<MockDDSTransmitter>();
    auto endpoint_factory = new MockEndPointFactory();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    
    // Set global pointers for forwarding
    Mock_DDSTransmitter = dds_transmitter.get();
    Mock_EndPointFactory = endpoint_factory;
    Mock_MWServiceManager = mw_service_manager.get();
    
    // Allow leak for endpoint_factory since it's managed by LifecycleManager after Initialize
    testing::Mock::AllowLeak(endpoint_factory);
    
    // Set expectations for Initialize
    EXPECT_CALL(*mw_service_manager, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*dds_transmitter, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*endpoint_factory, Initialize()).WillOnce(Return(0));
    EXPECT_CALL(*endpoint_factory, CreateEndPoints(_, _)).WillOnce(Return(0));
    
    // Create fresh manager and initialize
    TestableLifecycleManager fresh_mgr;
    EXPECT_TRUE(fresh_mgr.Initialize());
    
    // Now set up mocks for the components created by Initialize
    // We need to set the global pointers for StateManager, PolicyManager, StateMonitor
    auto state_manager = std::dynamic_pointer_cast<MockStateManager>(fresh_mgr.GetStateMgr());
    auto policy_manager = std::dynamic_pointer_cast<MockPolicyManager>(fresh_mgr.GetPolicy());
    auto state_monitor = std::dynamic_pointer_cast<MockStateMonitor>(fresh_mgr.GetMonitor());
    
    // Note: The real Initialize creates real objects, not mocks
    // So we need to inject mocks after Initialize for Start to work with mocks
    auto mock_state_manager = std::make_shared<MockStateManager>(fresh_mgr);
    auto mock_policy_manager = std::make_shared<MockPolicyManager>();
    auto mock_state_monitor = std::make_shared<MockStateMonitor>(fresh_mgr);
    
    Mock_StateManager = mock_state_manager.get();
    Mock_PolicyManager = mock_policy_manager.get();
    Mock_StateMonitor = mock_state_monitor.get();
    
    EXPECT_CALL(*mock_state_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*mock_policy_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*mw_service_manager, Start()).WillOnce(Return(true));
    EXPECT_CALL(*mock_state_monitor, Start()).WillOnce(Return(true));
    
    fresh_mgr.SetStateManager(mock_state_manager);
    fresh_mgr.SetPolicyManager(mock_policy_manager);
    fresh_mgr.SetStateMonitor(mock_state_monitor);
    
    EXPECT_TRUE(fresh_mgr.Start());
    
    // Clean up
    Mock_DDSTransmitter = nullptr;
    Mock_EndPointFactory = nullptr;
    Mock_MWServiceManager = nullptr;
    Mock_StateManager = nullptr;
    Mock_PolicyManager = nullptr;
    Mock_StateMonitor = nullptr;
}

TEST_F(LifecycleManagerTest, Initialize_MWServiceManagerInitializeFails_StillReturnsTrue) {
    // Create mock objects
    auto dds_transmitter = std::make_shared<MockDDSTransmitter>();
    auto endpoint_factory = new MockEndPointFactory();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    
    // Set global pointers for forwarding
    Mock_DDSTransmitter = dds_transmitter.get();
    Mock_EndPointFactory = endpoint_factory;
    Mock_MWServiceManager = mw_service_manager.get();
    
    // Allow leak for endpoint_factory since it's managed by LifecycleManager after Initialize
    testing::Mock::AllowLeak(endpoint_factory);
    
    // MWServiceManager::Initialize fails but Initialize still returns true
    // (current implementation doesn't check return values)
    EXPECT_CALL(*mw_service_manager, Initialize()).WillOnce(Return(false));
    EXPECT_CALL(*dds_transmitter, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*endpoint_factory, Initialize()).WillOnce(Return(0));
    EXPECT_CALL(*endpoint_factory, CreateEndPoints(_, _)).WillOnce(Return(0));
    
    TestableLifecycleManager fresh_mgr;
    // Initialize always returns true in current implementation
    EXPECT_TRUE(fresh_mgr.Initialize());
    
    // Clean up
    Mock_DDSTransmitter = nullptr;
    Mock_EndPointFactory = nullptr;
    Mock_MWServiceManager = nullptr;
}

TEST_F(LifecycleManagerTest, Initialize_DDSTransmitterInitializeFails_StillReturnsTrue) {
    // Create mock objects
    auto dds_transmitter = std::make_shared<MockDDSTransmitter>();
    auto endpoint_factory = new MockEndPointFactory();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    
    // Set global pointers for forwarding
    Mock_DDSTransmitter = dds_transmitter.get();
    Mock_EndPointFactory = endpoint_factory;
    Mock_MWServiceManager = mw_service_manager.get();
    
    // Allow leak for endpoint_factory since it's managed by LifecycleManager after Initialize
    testing::Mock::AllowLeak(endpoint_factory);
    
    // DDSTransmitter::Initialize fails but Initialize still returns true
    EXPECT_CALL(*mw_service_manager, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*dds_transmitter, Initialize()).WillOnce(Return(false));
    EXPECT_CALL(*endpoint_factory, Initialize()).WillOnce(Return(0));
    EXPECT_CALL(*endpoint_factory, CreateEndPoints(_, _)).WillOnce(Return(0));
    
    TestableLifecycleManager fresh_mgr;
    EXPECT_TRUE(fresh_mgr.Initialize());
    
    // Clean up
    Mock_DDSTransmitter = nullptr;
    Mock_EndPointFactory = nullptr;
    Mock_MWServiceManager = nullptr;
}

TEST_F(LifecycleManagerTest, Initialize_EndPointFactoryInitializeFails_StillReturnsTrue) {
    // Create mock objects
    auto dds_transmitter = std::make_shared<MockDDSTransmitter>();
    auto endpoint_factory = new MockEndPointFactory();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    
    // Set global pointers for forwarding
    Mock_DDSTransmitter = dds_transmitter.get();
    Mock_EndPointFactory = endpoint_factory;
    Mock_MWServiceManager = mw_service_manager.get();
    
    // Allow leak for endpoint_factory since it's managed by LifecycleManager after Initialize
    testing::Mock::AllowLeak(endpoint_factory);
    
    // EndPointFactory::Initialize fails (returns non-zero)
    EXPECT_CALL(*mw_service_manager, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*dds_transmitter, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*endpoint_factory, Initialize()).WillOnce(Return(-1));
    EXPECT_CALL(*endpoint_factory, CreateEndPoints(_, _)).WillOnce(Return(0));
    
    TestableLifecycleManager fresh_mgr;
    EXPECT_TRUE(fresh_mgr.Initialize());
    
    // Clean up
    Mock_DDSTransmitter = nullptr;
    Mock_EndPointFactory = nullptr;
    Mock_MWServiceManager = nullptr;
}

TEST_F(LifecycleManagerTest, Initialize_CreateEndPointsFails_StillReturnsTrue) {
    // Create mock objects
    auto dds_transmitter = std::make_shared<MockDDSTransmitter>();
    auto endpoint_factory = new MockEndPointFactory();
    auto mw_service_manager = std::make_shared<MockMWServiceManager>();
    
    // Set global pointers for forwarding
    Mock_DDSTransmitter = dds_transmitter.get();
    Mock_EndPointFactory = endpoint_factory;
    Mock_MWServiceManager = mw_service_manager.get();
    
    // Allow leak for endpoint_factory since it's managed by LifecycleManager after Initialize
    testing::Mock::AllowLeak(endpoint_factory);
    
    // CreateEndPoints fails (returns non-zero)
    EXPECT_CALL(*mw_service_manager, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*dds_transmitter, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*endpoint_factory, Initialize()).WillOnce(Return(0));
    EXPECT_CALL(*endpoint_factory, CreateEndPoints(_, _)).WillOnce(Return(-1));
    
    TestableLifecycleManager fresh_mgr;
    EXPECT_TRUE(fresh_mgr.Initialize());
    
    // Clean up
    Mock_DDSTransmitter = nullptr;
    Mock_EndPointFactory = nullptr;
    Mock_MWServiceManager = nullptr;
}

// ============================================================================
// Initialize() 가 등록하는 엔드포인트 핸들러
//
// 왜 필요했나:
//   LifecycleManager.cpp 의 미커버 62줄이 전부 이 람다들의 본문이었다 (라인
//   51~166). Initialize() 는 EndPointHandlers 를 채워 EndPointFactory 로
//   넘기는데, 람다가 실행되려면 DDS 메시지가 도착해야 해서 유닛 레인에서는
//   한 번도 불리지 않았다.
//
//   CreateEndPoints 의 두 번째 인자를 SaveArg 로 가로채면 DDS 없이 직접 부를
//   수 있다. 각 람다가 하는 일은 두 가지뿐이다 — (a) 협력자 포인터 null 가드,
//   (b) 해당 협력자로 위임. 두 경로를 모두 고정한다.
// ============================================================================

class LifecycleManagerHandlerTest : public LifecycleManagerTest {
  protected:
    void SetUp() override {
        LifecycleManagerTest::SetUp();

        dds_transmitter_    = std::make_shared<MockDDSTransmitter>();
        endpoint_factory_   = new MockEndPointFactory();
        mw_service_manager_ = std::make_shared<MockMWServiceManager>();

        Mock_DDSTransmitter   = dds_transmitter_.get();
        Mock_EndPointFactory  = endpoint_factory_;
        Mock_MWServiceManager = mw_service_manager_.get();
        // Initialize() 이후에는 LifecycleManager 가 소유한다.
        testing::Mock::AllowLeak(endpoint_factory_);

        ON_CALL(*mw_service_manager_, Initialize()).WillByDefault(Return(true));
        ON_CALL(*dds_transmitter_, Initialize()).WillByDefault(Return(true));
        ON_CALL(*endpoint_factory_, Initialize()).WillByDefault(Return(0));
        EXPECT_CALL(*endpoint_factory_, CreateEndPoints(_, _))
            .WillOnce(DoAll(SaveArg<1>(&handlers_), Return(0)));

        mgr_ = std::make_unique<TestableLifecycleManager>();
        ASSERT_TRUE(mgr_->Initialize());

        // Initialize() 가 만든 것은 실제 객체다. 그 메서드들은 링크 심을 통해
        // 아래 전역 mock 으로 흘러간다.
        state_manager_    = std::make_shared<::testing::NiceMock<MockStateManager>>(*mgr_);
        state_monitor_    = std::make_shared<::testing::NiceMock<MockStateMonitor>>(*mgr_);
        service_registry_ = std::make_shared<::testing::NiceMock<MockServiceRegistry>>();
        Mock_StateManager    = state_manager_.get();
        Mock_StateMonitor    = state_monitor_.get();
        Mock_ServiceRegistry = service_registry_.get();
    }

    void TearDown() override {
        mgr_.reset();
        Mock_DDSTransmitter   = nullptr;
        Mock_EndPointFactory  = nullptr;
        Mock_MWServiceManager = nullptr;
        LifecycleManagerTest::TearDown();
    }

    std::shared_ptr<MockDDSTransmitter>    dds_transmitter_;
    MockEndPointFactory*                   endpoint_factory_ = nullptr;
    std::shared_ptr<MockMWServiceManager>  mw_service_manager_;
    std::unique_ptr<TestableLifecycleManager> mgr_;
    EndPointHandlers                       handlers_;

    std::shared_ptr<::testing::NiceMock<MockStateManager>>    state_manager_;
    std::shared_ptr<::testing::NiceMock<MockStateMonitor>>    state_monitor_;
    std::shared_ptr<::testing::NiceMock<MockServiceRegistry>> service_registry_;
};

// Initialize() 가 핸들러를 하나도 빠짐없이 채웠는지부터 본다. 비어 있는
// std::function 을 호출하면 std::bad_function_call 이므로, 엔드포인트가 새로
// 늘었는데 배선을 빠뜨리면 여기서 걸린다.
TEST_F(LifecycleManagerHandlerTest, AllHandlersAreWired) {
    EXPECT_TRUE(static_cast<bool>(handlers_.on_start));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_stop));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_set_mode));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_get_state_resp));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_notify_state));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_update_list));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_notify_container_state));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_start_container_resp));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_stop_container_resp));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_ctrl_start_service));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_ctrl_stop_service));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_ctrl_set_service_mode));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_ctrl_start_container));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_ctrl_stop_container));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_set_recovery_policy_resp));
    EXPECT_TRUE(static_cast<bool>(handlers_.on_ctrl_set_recovery_policy));
}

// ── StateManager 로 가는 것들 ────────────────────────────────────────────────
// 메시지 포인터는 그대로 통과만 하므로 nullptr 로도 배선을 확인할 수 있다.

TEST_F(LifecycleManagerHandlerTest, ServiceResponsesForwardToStateManager) {
    EXPECT_CALL(*state_manager_, OnStart(nullptr));
    EXPECT_CALL(*state_manager_, OnStop(nullptr));
    EXPECT_CALL(*state_manager_, SetServiceMode(nullptr)).WillOnce(Return(true));

    handlers_.on_start(nullptr);
    handlers_.on_stop(nullptr);
    handlers_.on_set_mode(nullptr);
}

TEST_F(LifecycleManagerHandlerTest, ContainerResponsesForwardToStateManager) {
    EXPECT_CALL(*state_manager_, OnStartContainer(nullptr));
    EXPECT_CALL(*state_manager_, OnStopContainer(nullptr));
    EXPECT_CALL(*state_manager_, OnSetRecoveryPolicy(nullptr));

    handlers_.on_start_container_resp(nullptr);
    handlers_.on_stop_container_resp(nullptr);
    handlers_.on_set_recovery_policy_resp(nullptr);
}

TEST_F(LifecycleManagerHandlerTest, CtrlRequestsForwardToStateManager) {
    EXPECT_CALL(*state_manager_, HandleCtrlStartService(nullptr));
    EXPECT_CALL(*state_manager_, HandleCtrlStopService(nullptr));
    EXPECT_CALL(*state_manager_, HandleCtrlSetServiceMode(nullptr));
    EXPECT_CALL(*state_manager_, HandleCtrlStartContainer(nullptr));
    EXPECT_CALL(*state_manager_, HandleCtrlStopContainer(nullptr));
    EXPECT_CALL(*state_manager_, HandleCtrlSetRecoveryPolicy(nullptr));

    handlers_.on_ctrl_start_service(nullptr);
    handlers_.on_ctrl_stop_service(nullptr);
    handlers_.on_ctrl_set_service_mode(nullptr);
    handlers_.on_ctrl_start_container(nullptr);
    handlers_.on_ctrl_stop_container(nullptr);
    handlers_.on_ctrl_set_recovery_policy(nullptr);
}

// ── StateMonitor / ServiceRegistry 로 가는 것들 ──────────────────────────────

TEST_F(LifecycleManagerHandlerTest, StateReportsForwardToStateMonitor) {
    EXPECT_CALL(*state_monitor_,
                UpdateServicesState(::testing::A<lifecyclemgr::GetServiceStateResponse*>()));
    EXPECT_CALL(*state_monitor_,
                UpdateServicesState(::testing::A<lifecyclemgr::NotifyServiceState*>()));

    handlers_.on_get_state_resp(nullptr);
    handlers_.on_notify_state(nullptr);
}

TEST_F(LifecycleManagerHandlerTest, UpdateServiceListForwardsToRegistry) {
    EXPECT_CALL(*service_registry_, UpdateServiceList(nullptr)).WillOnce(Return(true));
    handlers_.on_update_list(nullptr);
}

// ── null 가드 ───────────────────────────────────────────────────────────────
// 협력자가 사라진 뒤에 메시지가 도착해도 (종료 순서상 실제로 가능하다)
// 어떤 핸들러도 역참조하면 안 된다.

TEST_F(LifecycleManagerHandlerTest, HandlersAreNoOpWhenCollaboratorsAreGone) {
    mgr_->SetStateManager(nullptr);
    mgr_->SetStateMonitor(nullptr);
    mgr_->SetServiceRegistry(nullptr);

    EXPECT_CALL(*state_manager_, OnStart(_)).Times(0);
    EXPECT_CALL(*state_monitor_,
                UpdateServicesState(::testing::A<lifecyclemgr::NotifyServiceState*>())).Times(0);
    EXPECT_CALL(*service_registry_, UpdateServiceList(_)).Times(0);

    EXPECT_NO_THROW({
        handlers_.on_start(nullptr);
        handlers_.on_stop(nullptr);
        handlers_.on_set_mode(nullptr);
        handlers_.on_get_state_resp(nullptr);
        handlers_.on_notify_state(nullptr);
        handlers_.on_update_list(nullptr);
        handlers_.on_notify_container_state(nullptr);
        handlers_.on_start_container_resp(nullptr);
        handlers_.on_stop_container_resp(nullptr);
        handlers_.on_ctrl_start_service(nullptr);
        handlers_.on_ctrl_stop_service(nullptr);
        handlers_.on_ctrl_set_service_mode(nullptr);
        handlers_.on_ctrl_start_container(nullptr);
        handlers_.on_ctrl_stop_container(nullptr);
        handlers_.on_set_recovery_policy_resp(nullptr);
        handlers_.on_ctrl_set_recovery_policy(nullptr);
    });
}

// ── NotifyContainerState — 유일하게 본문 로직이 있는 핸들러 ──────────────────

TEST_F(LifecycleManagerHandlerTest, NotifyContainerStateIgnoresNullMessage) {
    EXPECT_CALL(*service_registry_, UpdateContainerState(_, _, _)).Times(0);
    EXPECT_NO_THROW(handlers_.on_notify_container_state(nullptr));
}

TEST_F(LifecycleManagerHandlerTest, NotifyContainerStatePublishesOnRealTransition) {
    lifecyclemgr::NotifyContainerState msg;
    msg.container_list({"web", "db"});
    msg.container_state_list({lifecyclemgr::ContainerState::CONTAINER_RUNNING,
                              lifecyclemgr::ContainerState::CONTAINER_STOPPED});
    msg.exit_code_list({0, 137});

    EXPECT_CALL(*service_registry_,
                UpdateContainerState("web", lifecycle_manager::ContainerState::CONTAINER_RUNNING, 0))
        .WillOnce(Return(false));
    EXPECT_CALL(*service_registry_,
                UpdateContainerState("db", lifecycle_manager::ContainerState::CONTAINER_STOPPED, 137))
        .WillOnce(Return(true));
    // 하나라도 실제로 바뀌었으면 하트비트를 기다리지 않고 즉시 발행한다.
    EXPECT_CALL(*state_monitor_, NotifyLifecycleDiagnosticInfo(true));

    handlers_.on_notify_container_state(&msg);
}

TEST_F(LifecycleManagerHandlerTest, NotifyContainerStateStaysQuietWhenNothingChanged) {
    lifecyclemgr::NotifyContainerState msg;
    msg.container_list({"web"});
    msg.container_state_list({lifecyclemgr::ContainerState::CONTAINER_RUNNING});
    msg.exit_code_list({0});

    EXPECT_CALL(*service_registry_, UpdateContainerState(_, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*state_monitor_, NotifyLifecycleDiagnosticInfo(_)).Times(0);

    handlers_.on_notify_container_state(&msg);
}

// 세 배열은 병렬이라고 약속돼 있지만 길이가 어긋난 채로 올 수 있다 (송신 측
// 버그, 부분 직렬화). 짧은 쪽을 인덱싱하지 않고 기본값으로 메우는지 본다.
TEST_F(LifecycleManagerHandlerTest, NotifyContainerStateToleratesShorterParallelArrays) {
    lifecyclemgr::NotifyContainerState msg;
    msg.container_list({"web", "db"});
    msg.container_state_list({lifecyclemgr::ContainerState::CONTAINER_RUNNING}); // 1개뿐
    msg.exit_code_list({});                                                      // 비어 있음

    EXPECT_CALL(*service_registry_,
                UpdateContainerState("web", lifecycle_manager::ContainerState::CONTAINER_RUNNING, 0))
        .WillOnce(Return(false));
    // 상태·종료코드가 없는 두 번째는 CS_UNKNOWN / 0 으로 채워져야 한다.
    EXPECT_CALL(*service_registry_,
                UpdateContainerState("db", lifecycle_manager::ContainerState::CS_UNKNOWN, 0))
        .WillOnce(Return(false));

    EXPECT_NO_THROW(handlers_.on_notify_container_state(&msg));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
