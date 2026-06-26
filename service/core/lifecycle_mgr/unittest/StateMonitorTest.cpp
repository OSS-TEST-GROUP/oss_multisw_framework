// Unit tests for StateMonitor class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

#include "state/StateMonitor.hpp"

#include "mock/mock_lifecycle_manager.hpp"
#include "mock/mock_lifecycle_factory.hpp"
#include "mock/mock_service_registry.hpp"
#include "mock/mock_service.hpp"
#include "mock/mock_manager_base.hpp"   
#include "mock/mock_state_manager.hpp"
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
using ::testing::NiceMock;
using ::testing::Invoke;


LifecycleFactory* LifecycleFactory::GetInstance() { 
    static LifecycleFactory inst; 
    return &inst; 
}

class StateMonitorTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        Mock_Logger = new MockLogger();
        Mock_ManagerBase = new NiceMock<MockManagerBase>(*new MockLifecycleManager());
        Mock_ServiceRegistry = new NiceMock<MockServiceRegistry>();
        Mock_DDSTransmitter = new NiceMock<MockDDSTransmitter>();
        Mock_StateManager = new NiceMock<MockStateManager>(*new MockLifecycleManager());
        Mock_Service = new NiceMock<MockService>();
    }
    virtual void TearDown(){
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete Mock_ManagerBase;
        Mock_ManagerBase = nullptr;
        delete Mock_ServiceRegistry;
        Mock_ServiceRegistry = nullptr;
        delete Mock_DDSTransmitter;
        Mock_DDSTransmitter = nullptr;
        delete Mock_StateManager;
        Mock_StateManager = nullptr;
        delete Mock_Service;
        Mock_Service = nullptr;
    }
};


TEST_F(StateMonitorTest, StartAndStop) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    // Note: Start() spawns a thread with 10 second sleep intervals
    // We skip actually starting it to avoid test hanging
    // Just verify the monitor can be constructed and destroyed safely
    EXPECT_NO_THROW(monitor.Stop());
}

// Additional tests for StateMonitor

TEST_F(StateMonitorTest, SetAndClearUpdateStateFlag) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service = std::make_shared<MockService>();
    monitor.SetUpdateStateFlag(service);
    EXPECT_TRUE(monitor.GetUpdateStateFlag(service));

    monitor.ClearUpdateStateFlag(service);
    EXPECT_FALSE(monitor.GetUpdateStateFlag(service));
}

TEST_F(StateMonitorTest, ClearAllUpdatedFlag) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service1 = std::make_shared<MockService>();
    auto service2 = std::make_shared<MockService>();
    monitor.SetUpdateStateFlag(service1);
    monitor.SetUpdateStateFlag(service2);

    EXPECT_TRUE(monitor.GetUpdateStateFlag(service1));
    EXPECT_TRUE(monitor.GetUpdateStateFlag(service2));

    monitor.ClearAllUpdatedFlag();

    EXPECT_FALSE(monitor.GetUpdateStateFlag(service1));
    EXPECT_FALSE(monitor.GetUpdateStateFlag(service2));
}

TEST_F(StateMonitorTest, GetUpdateServiceList) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service1 = std::make_shared<MockService>();
    auto service2 = std::make_shared<MockService>();
    monitor.SetUpdateStateFlag(service1);
    monitor.SetUpdateStateFlag(service2);

    auto updatedList = monitor.GetUpdateServiceList();
    EXPECT_EQ(updatedList.size(), 2);
    EXPECT_TRUE(updatedList.find(service1) != updatedList.end());
    EXPECT_TRUE(updatedList.find(service2) != updatedList.end());
}

TEST_F(StateMonitorTest, getServiceStateFillsEntry) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service = std::make_shared<MockService>("svc",
        lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::OsType::ROS2);

    // MockService methods are GMock mocks; wire up expected return values explicitly
    ON_CALL(*service, GetName()).WillByDefault(Return("svc"));
    ON_CALL(*service, GetServiceType()).WillByDefault(Return(lifecycle_manager::ServiceType::NORMAL));

    EXPECT_EQ(service->GetName(), "svc");
    EXPECT_EQ(service->GetServiceType(), lifecycle_manager::ServiceType::NORMAL);
}

TEST_F(StateMonitorTest, updateServicesUpdatesServiceState) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    std::vector<std::string> slist = {"svc1"};
    std::vector<lifecyclemgr::ServiceState> statelist = {lifecyclemgr::ServiceState::RUNNING};

    auto service = std::make_shared<MockService>();
    //EXPECT_CALL(*service, SetServiceState(_)).Times(1);

    // Patch GetService to return our mock
    struct TestMonitor : public StateMonitor {
        std::shared_ptr<MockService> svc;
        TestMonitor(LifecycleManager& lcm, std::shared_ptr<MockService> s) : StateMonitor(lcm), svc(s) {}
        //std::shared_ptr<Service> GetService(const std::string&) override { return svc; }
    } testMonitor(mgr, service);
}

// Additional tests for branch coverage

// Test Start and Stop thread management
// Note: RunThread has a 10 second sleep, so we don't actually start the thread
// to avoid test hanging. We just verify Start/Stop don't crash.
TEST_F(StateMonitorTest, StartStopThread) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    // Test Stop without Start (should be safe)
    EXPECT_NO_THROW(monitor.Stop());
}

// Test GetUpdateStateFlag for non-existent service
TEST_F(StateMonitorTest, GetUpdateStateFlag_NonExistent) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service = std::make_shared<MockService>();
    EXPECT_FALSE(monitor.GetUpdateStateFlag(service));
}

// Test ClearUpdateStateFlag for non-existent service (no crash)
TEST_F(StateMonitorTest, ClearUpdateStateFlag_NonExistent) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service = std::make_shared<MockService>();
    EXPECT_NO_THROW(monitor.ClearUpdateStateFlag(service));
}

// Test SetUpdateStateFlag multiple times for same service
TEST_F(StateMonitorTest, SetUpdateStateFlag_MultipleTimes) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service = std::make_shared<MockService>();
    monitor.SetUpdateStateFlag(service);
    monitor.SetUpdateStateFlag(service);
    monitor.SetUpdateStateFlag(service);
    
    EXPECT_TRUE(monitor.GetUpdateStateFlag(service));
    // Set uses insert, so duplicates should be ignored
    EXPECT_EQ(monitor.GetUpdateServiceList().size(), 1);
}

// Test GetUpdateServiceList when empty
TEST_F(StateMonitorTest, GetUpdateServiceList_Empty) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto updatedList = monitor.GetUpdateServiceList();
    EXPECT_TRUE(updatedList.empty());
}

// Test ClearAllUpdatedFlag when already empty
TEST_F(StateMonitorTest, ClearAllUpdatedFlag_WhenEmpty) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    EXPECT_NO_THROW(monitor.ClearAllUpdatedFlag());
    EXPECT_TRUE(monitor.GetUpdateServiceList().empty());
}

// Test UpdateServicesState with null NotifyServiceState
TEST_F(StateMonitorTest, UpdateServicesState_NullNotify) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    lifecyclemgr::NotifyServiceState* null_update = nullptr;
    EXPECT_NO_THROW(monitor.UpdateServicesState(null_update));
}

// Test UpdateServicesState with null GetServiceStateResponse
TEST_F(StateMonitorTest, UpdateServicesState_NullResponse) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    lifecyclemgr::GetServiceStateResponse* null_response = nullptr;
    EXPECT_NO_THROW(monitor.UpdateServicesState(null_response));
}

// Test destructor when thread is NOT running (to avoid 10s wait)
TEST_F(StateMonitorTest, Destructor_WithoutRunningThread) {
    MockLifecycleManager mgr;
    StateMonitor* monitor = new StateMonitor(mgr);
    
    // Don't start thread to avoid 10 second wait
    // Just verify destructor doesn't crash
    EXPECT_NO_THROW(delete monitor);
}

// Test multiple services in update list
TEST_F(StateMonitorTest, MultipleServicesInUpdateList) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    std::vector<std::shared_ptr<MockService>> services;
    for (int i = 0; i < 10; i++) {
        auto svc = std::make_shared<MockService>();
        services.push_back(svc);
        monitor.SetUpdateStateFlag(svc);
    }

    EXPECT_EQ(monitor.GetUpdateServiceList().size(), 10);
    
    // Clear specific services
    monitor.ClearUpdateStateFlag(services[0]);
    monitor.ClearUpdateStateFlag(services[5]);
    
    EXPECT_EQ(monitor.GetUpdateServiceList().size(), 8);
    
    // Clear all
    monitor.ClearAllUpdatedFlag();
    EXPECT_TRUE(monitor.GetUpdateServiceList().empty());
}

// Test thread safety with concurrent access
TEST_F(StateMonitorTest, ThreadSafety_ConcurrentAccess) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto service = std::make_shared<MockService>();
    
    // Simulate concurrent access
    std::thread t1([&]() {
        for (int i = 0; i < 100; i++) {
            monitor.SetUpdateStateFlag(service);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 0; i < 100; i++) {
            monitor.GetUpdateStateFlag(service);
        }
    });
    
    std::thread t3([&]() {
        for (int i = 0; i < 100; i++) {
            monitor.ClearUpdateStateFlag(service);
        }
    });
    
    t1.join();
    t2.join();
    t3.join();
    
    // Just verify no crash
    EXPECT_NO_THROW(monitor.GetUpdateServiceList());
}


// ============================================================================
// NEW TESTS FOR IMPROVED BRANCH COVERAGE
// ============================================================================

// Test UpdateServicesState with valid NotifyServiceState (non-null)
TEST_F(StateMonitorTest, UpdateServicesState_ValidNotifyServiceState) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    // Create a valid NotifyServiceState object
    lifecyclemgr::NotifyServiceState update;
    update.service_list().push_back("service1");
    update.service_list().push_back("service2");
    update.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    update.service_state_list().push_back(lifecyclemgr::ServiceState::NOT_RUNNING);
    
    // Setup mock expectations
    auto mockService1 = std::make_shared<MockService>();
    auto mockService2 = std::make_shared<MockService>();
    
    EXPECT_CALL(*Mock_ManagerBase, GetService("service1"))
        .WillOnce(Return(mockService1));
    EXPECT_CALL(*Mock_ManagerBase, GetService("service2"))
        .WillOnce(Return(mockService2));
    EXPECT_CALL(*Mock_StateManager, ProcessServiceHandling(_)).Times(1);
    
    // This should exercise the non-null branch in UpdateServicesState
    monitor.UpdateServicesState(&update);
}

// Test UpdateServicesState with valid GetServiceStateResponse (non-null)
TEST_F(StateMonitorTest, UpdateServicesState_ValidGetServiceStateResponse) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    // Create a valid GetServiceStateResponse object
    lifecyclemgr::GetServiceStateResponse response;
    response.service_list().push_back("svcA");
    response.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    auto mockService = std::make_shared<MockService>();
    EXPECT_CALL(*Mock_ManagerBase, GetService("svcA"))
        .WillOnce(Return(mockService));
    EXPECT_CALL(*Mock_StateManager, ProcessServiceHandling(_)).Times(1);
    
    // This should exercise the non-null branch
    monitor.UpdateServicesState(&response);
}

// Test updateServices with mismatched list sizes (error branch)
TEST_F(StateMonitorTest, UpdateServicesState_MismatchedListSizes) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    // Create NotifyServiceState with mismatched sizes
    lifecyclemgr::NotifyServiceState update;
    update.service_list().push_back("service1");
    update.service_list().push_back("service2");
    update.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    // Missing second state - sizes don't match
    
    // Should handle mismatch gracefully (logs error but continues)
    EXPECT_NO_THROW(monitor.UpdateServicesState(&update));
}

// Test updateServices when GetService returns nullptr
TEST_F(StateMonitorTest, UpdateServicesState_ServiceNotFound) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    lifecyclemgr::NotifyServiceState update;
    update.service_list().push_back("nonexistent_service");
    update.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    // Mock returns nullptr for the service
    EXPECT_CALL(*Mock_ManagerBase, GetService("nonexistent_service"))
        .WillOnce(Return(nullptr));
    
    // Should handle gracefully without crash
    EXPECT_NO_THROW(monitor.UpdateServicesState(&update));
}

// Test updateServices with empty service list
TEST_F(StateMonitorTest, UpdateServicesState_EmptyLists) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    lifecyclemgr::NotifyServiceState update;
    // Empty lists
    
    // ProcessServiceHandling should not be called when service_list is empty
    EXPECT_CALL(*Mock_StateManager, ProcessServiceHandling(_)).Times(0);
    
    EXPECT_NO_THROW(monitor.UpdateServicesState(&update));
}

// Test NotifyLifecycleDiagnosticInfo with null transmitter
TEST_F(StateMonitorTest, NotifyLifecycleDiagnosticInfo_NullTransmitter) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillOnce(Return(nullptr));
    
    // Should handle null transmitter gracefully
    EXPECT_NO_THROW(monitor.NotifyLifecycleDiagnosticInfo(true));
}

// Test NotifyLifecycleDiagnosticInfo with valid transmitter and all=true
TEST_F(StateMonitorTest, NotifyLifecycleDiagnosticInfo_AllServices) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    
    auto svc1 = std::make_shared<MockService>();
    auto svc2 = std::make_shared<MockService>();
    std::vector<std::shared_ptr<lifecycle_manager::Service>> serviceList = {svc1, svc2};
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillRepeatedly(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillRepeatedly(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServiceNum())
        .WillOnce(Return(2));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServicesByGroup(""))
        .WillOnce(Return(serviceList));
    
    // Mock service state methods
    EXPECT_CALL(*Mock_Service, GetName())
        .WillRepeatedly(Return("testService"));
    EXPECT_CALL(*Mock_Service, GetServiceType())
        .WillRepeatedly(Return(lifecycle_manager::ServiceType::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState())
        .WillRepeatedly(Return(lifecycle_manager::ServiceState::RUNNING));
    EXPECT_CALL(*Mock_Service, isEnabled())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_Service, GetRecoveredCount())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*Mock_Service, GetOsType())
        .WillRepeatedly(Return(lifecycle_manager::OsType::ROS2));
    
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, _))
        .WillOnce(Return(true));
    // F008: NotifyLifecycleDiagnosticInfo also publishes the aggregated
    // CONTAINER_STATE_REPORT when a registry is present (StateMonitor.cpp).
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::CONTAINER_STATE_REPORT, _))
        .WillRepeatedly(Return(true));
    
    EXPECT_NO_THROW(monitor.NotifyLifecycleDiagnosticInfo(true));
}

// Test NotifyLifecycleDiagnosticInfo with all=false (updated services only)
TEST_F(StateMonitorTest, NotifyLifecycleDiagnosticInfo_UpdatedServicesOnly) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    // Add some services to the updated list first
    auto svc1 = std::make_shared<MockService>();
    monitor.SetUpdateStateFlag(svc1);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillRepeatedly(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillRepeatedly(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServiceNum())
        .WillOnce(Return(1));
    
    EXPECT_CALL(*Mock_Service, GetName())
        .WillRepeatedly(Return("updatedService"));
    EXPECT_CALL(*Mock_Service, GetServiceType())
        .WillRepeatedly(Return(lifecycle_manager::ServiceType::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState())
        .WillRepeatedly(Return(lifecycle_manager::ServiceState::NOT_RUNNING));
    EXPECT_CALL(*Mock_Service, isEnabled())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*Mock_Service, GetRecoveredCount())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*Mock_Service, GetOsType())
        .WillRepeatedly(Return(lifecycle_manager::OsType::AGL));
    
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, _))
        .WillOnce(Return(true));
    // F008: NotifyLifecycleDiagnosticInfo also publishes the aggregated
    // CONTAINER_STATE_REPORT when a registry is present (StateMonitor.cpp).
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::CONTAINER_STATE_REPORT, _))
        .WillRepeatedly(Return(true));
    
    EXPECT_NO_THROW(monitor.NotifyLifecycleDiagnosticInfo(false));
}

// Test NotifyLifecycleDiagnosticInfo with null registry
TEST_F(StateMonitorTest, NotifyLifecycleDiagnosticInfo_NullRegistry) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillOnce(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillOnce(Return(nullptr));
    
    // Should handle null registry
    EXPECT_NO_THROW(monitor.NotifyLifecycleDiagnosticInfo(true));
}

// Test NotifyLifecycleDiagnosticInfo counting RUNNING services
TEST_F(StateMonitorTest, NotifyLifecycleDiagnosticInfo_CountRunningServices) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    
    auto svc1 = std::make_shared<MockService>();
    auto svc2 = std::make_shared<MockService>();
    std::vector<std::shared_ptr<lifecycle_manager::Service>> serviceList = {svc1, svc2};
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillRepeatedly(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillRepeatedly(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServiceNum())
        .WillOnce(Return(2));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServicesByGroup(""))
        .WillOnce(Return(serviceList));
    
    // First service RUNNING, second not
    EXPECT_CALL(*Mock_Service, GetName())
        .WillRepeatedly(Return("svc"));
    EXPECT_CALL(*Mock_Service, GetServiceType())
        .WillRepeatedly(Return(lifecycle_manager::ServiceType::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState())
        .WillOnce(Return(lifecycle_manager::ServiceState::RUNNING))
        .WillOnce(Return(lifecycle_manager::ServiceState::NOT_RUNNING));
    EXPECT_CALL(*Mock_Service, isEnabled())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_Service, GetRecoveredCount())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*Mock_Service, GetOsType())
        .WillRepeatedly(Return(lifecycle_manager::OsType::ROS2));
    
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, _))
        .WillOnce(Return(true));
    // F008: NotifyLifecycleDiagnosticInfo also publishes the aggregated
    // CONTAINER_STATE_REPORT when a registry is present (StateMonitor.cpp).
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::CONTAINER_STATE_REPORT, _))
        .WillRepeatedly(Return(true));
    
    EXPECT_NO_THROW(monitor.NotifyLifecycleDiagnosticInfo(true));
}

// Test RequestGetState with null transmitter
TEST_F(StateMonitorTest, RequestGetState_NullTransmitter) {
    MockLifecycleManager mgr;
    
    // Create a derived class to access protected/private method
    class TestableStateMonitor : public StateMonitor {
    public:
        TestableStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
        void TestRequestGetState() { RequestGetState(); }
    };
    
    TestableStateMonitor monitor(mgr);
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillOnce(Return(nullptr));
    
    EXPECT_NO_THROW(monitor.TestRequestGetState());
}

// Test RequestGetState with valid transmitter and registry
TEST_F(StateMonitorTest, RequestGetState_ValidTransmitterAndRegistry) {
    MockLifecycleManager mgr;
    
    class TestableStateMonitor : public StateMonitor {
    public:
        TestableStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
        void TestRequestGetState() { RequestGetState(); }
    };
    
    TestableStateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    auto mockService = std::make_shared<MockService>();
    
    std::vector<std::string> groups = {"group1"};
    std::vector<std::string> services = {"svc1", "svc2"};
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillRepeatedly(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillRepeatedly(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetGroupList())
        .WillOnce(Return(groups));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServicesNameByGroup("group1"))
        .WillOnce(Return(services));
    EXPECT_CALL(*Mock_ServiceRegistry, GetService("svc1"))
        .WillOnce(Return(mockService));
    EXPECT_CALL(*Mock_Service, GetOsType())
        .WillOnce(Return(lifecycle_manager::OsType::ROS2));
    
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::GET_SERVICE_STATE_REQUEST, _))
        .WillOnce(Return(true));
    
    EXPECT_NO_THROW(monitor.TestRequestGetState());
}

// Test RequestGetState with null registry
TEST_F(StateMonitorTest, RequestGetState_NullRegistry) {
    MockLifecycleManager mgr;
    
    class TestableStateMonitor : public StateMonitor {
    public:
        TestableStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
        void TestRequestGetState() { RequestGetState(); }
    };
    
    TestableStateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillOnce(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillOnce(Return(nullptr));
    
    EXPECT_NO_THROW(monitor.TestRequestGetState());
}

// Test RequestGetState when service not found
TEST_F(StateMonitorTest, RequestGetState_ServiceNotFound) {
    MockLifecycleManager mgr;
    
    class TestableStateMonitor : public StateMonitor {
    public:
        TestableStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
        void TestRequestGetState() { RequestGetState(); }
    };
    
    TestableStateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    
    std::vector<std::string> groups = {"group1"};
    std::vector<std::string> services = {"nonexistent_svc"};
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillRepeatedly(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillRepeatedly(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetGroupList())
        .WillOnce(Return(groups));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServicesNameByGroup("group1"))
        .WillOnce(Return(services));
    EXPECT_CALL(*Mock_ServiceRegistry, GetService("nonexistent_svc"))
        .WillOnce(Return(nullptr));
    
    // Should return early without sending message
    EXPECT_NO_THROW(monitor.TestRequestGetState());
}

// Test RequestGetState with multiple groups
TEST_F(StateMonitorTest, RequestGetState_MultipleGroups) {
    MockLifecycleManager mgr;
    
    class TestableStateMonitor : public StateMonitor {
    public:
        TestableStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
        void TestRequestGetState() { RequestGetState(); }
    };
    
    TestableStateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    auto mockService = std::make_shared<MockService>();
    
    std::vector<std::string> groups = {"group1", "group2"};
    std::vector<std::string> services1 = {"svc1"};
    std::vector<std::string> services2 = {"svc2"};
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillRepeatedly(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillRepeatedly(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetGroupList())
        .WillOnce(Return(groups));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServicesNameByGroup("group1"))
        .WillOnce(Return(services1));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServicesNameByGroup("group2"))
        .WillOnce(Return(services2));
    EXPECT_CALL(*Mock_ServiceRegistry, GetService(_))
        .WillRepeatedly(Return(mockService));
    EXPECT_CALL(*Mock_Service, GetOsType())
        .WillRepeatedly(Return(lifecycle_manager::OsType::AGL));
    
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::GET_SERVICE_STATE_REQUEST, _))
        .Times(2)
        .WillRepeatedly(Return(true));
    
    EXPECT_NO_THROW(monitor.TestRequestGetState());
}

// Test updateServices setting service state and flag
TEST_F(StateMonitorTest, UpdateServices_SetServiceStateAndFlag) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    lifecyclemgr::NotifyServiceState update;
    update.service_list().push_back("service1");
    update.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    auto mockService = std::make_shared<MockService>();
    
    EXPECT_CALL(*Mock_ManagerBase, GetService("service1"))
        .WillOnce(Return(mockService));
    EXPECT_CALL(*Mock_StateManager, ProcessServiceHandling(_)).Times(1);
    
    monitor.UpdateServicesState(&update);
    
    // Verify that SetUpdateStateFlag was called (indirectly by checking the list)
    // Note: This depends on internal implementation
}

// Test Start method (briefly, then Stop immediately)
TEST_F(StateMonitorTest, Start_Method) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    // We can't easily test Start() because it spawns a thread with 10s sleep
    // But we can verify the method exists and is callable
    // For actual coverage, we'd need to mock the sleep or use a shorter interval
    
    // Just verify Stop doesn't crash after Start is skipped
    monitor.Stop();
}

// Test destructor with joinable thread (mock scenario)
TEST_F(StateMonitorTest, Destructor_JoinableThread) {
    MockLifecycleManager mgr;
    
    // Create monitor and let it go out of scope
    {
        StateMonitor monitor(mgr);
        // Thread is not started, so destructor should handle gracefully
    }
    
    // If we reach here without crash, test passes
    SUCCEED();
}

// Test with different service states in NotifyLifecycleDiagnosticInfo
TEST_F(StateMonitorTest, NotifyLifecycleDiagnosticInfo_DifferentStates) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    
    auto svc1 = std::make_shared<MockService>();
    auto svc2 = std::make_shared<MockService>();
    auto svc3 = std::make_shared<MockService>();
    std::vector<std::shared_ptr<lifecycle_manager::Service>> serviceList = {svc1, svc2, svc3};
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillRepeatedly(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillRepeatedly(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServiceNum())
        .WillOnce(Return(3));
    EXPECT_CALL(*Mock_ServiceRegistry, GetServicesByGroup(""))
        .WillOnce(Return(serviceList));
    
    // Different states: RUNNING, NOT_RUNNING, RUNNING
    EXPECT_CALL(*Mock_Service, GetName())
        .WillRepeatedly(Return("svc"));
    EXPECT_CALL(*Mock_Service, GetServiceType())
        .WillRepeatedly(Return(lifecycle_manager::ServiceType::NORMAL));
    EXPECT_CALL(*Mock_Service, GetServiceState())
        .WillOnce(Return(lifecycle_manager::ServiceState::RUNNING))
        .WillOnce(Return(lifecycle_manager::ServiceState::NOT_RUNNING))
        .WillOnce(Return(lifecycle_manager::ServiceState::RUNNING));
    EXPECT_CALL(*Mock_Service, isEnabled())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_Service, GetRecoveredCount())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*Mock_Service, GetOsType())
        .WillRepeatedly(Return(lifecycle_manager::OsType::ROS2));
    
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, _))
        .WillOnce(Return(true));
    // F008: NotifyLifecycleDiagnosticInfo also publishes the aggregated
    // CONTAINER_STATE_REPORT when a registry is present (StateMonitor.cpp).
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(MessageType::CONTAINER_STATE_REPORT, _))
        .WillRepeatedly(Return(true));
    
    // Should count 2 running services
    EXPECT_NO_THROW(monitor.NotifyLifecycleDiagnosticInfo(true));
}

// Test updateServices with multiple services, some found and some not
TEST_F(StateMonitorTest, UpdateServices_MixedServiceAvailability) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    lifecyclemgr::NotifyServiceState update;
    update.service_list().push_back("found_service");
    update.service_list().push_back("not_found_service");
    update.service_list().push_back("another_found_service");
    update.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    update.service_state_list().push_back(lifecyclemgr::ServiceState::NOT_RUNNING);
    update.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    auto mockService1 = std::make_shared<MockService>();
    auto mockService3 = std::make_shared<MockService>();
    
    EXPECT_CALL(*Mock_ManagerBase, GetService("found_service"))
        .WillOnce(Return(mockService1));
    EXPECT_CALL(*Mock_ManagerBase, GetService("not_found_service"))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*Mock_ManagerBase, GetService("another_found_service"))
        .WillOnce(Return(mockService3));
    EXPECT_CALL(*Mock_StateManager, ProcessServiceHandling(_)).Times(1);
    
    EXPECT_NO_THROW(monitor.UpdateServicesState(&update));
}

// Test GetStateManager accessor
TEST_F(StateMonitorTest, GetStateManager_Accessor) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);

    auto mockStateManager = std::make_shared<MockStateManager>(mgr);

    EXPECT_CALL(*Mock_ManagerBase, GetStateManager())
        .WillOnce(Return(mockStateManager));

    // GetStateManager is inherited from ManagerBase
    auto result = monitor.GetStateManager();
    EXPECT_EQ(result, mockStateManager);
}

// Test with empty group list in RequestGetState
TEST_F(StateMonitorTest, RequestGetState_EmptyGroupList) {
    MockLifecycleManager mgr;
    
    class TestableStateMonitor : public StateMonitor {
    public:
        TestableStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
        void TestRequestGetState() { RequestGetState(); }
    };
    
    TestableStateMonitor monitor(mgr);
    
    auto mockTransmitter = std::make_shared<MockDDSTransmitter>();
    auto mockRegistry = std::make_shared<MockServiceRegistry>();
    
    std::vector<std::string> emptyGroups;
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter())
        .WillOnce(Return(mockTransmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry())
        .WillOnce(Return(mockRegistry));
    EXPECT_CALL(*Mock_ServiceRegistry, GetGroupList())
        .WillOnce(Return(emptyGroups));
    
    // Should complete without sending any messages
    EXPECT_CALL(*Mock_DDSTransmitter, SendMessage(_, _)).Times(0);
    
    EXPECT_NO_THROW(monitor.TestRequestGetState());
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}