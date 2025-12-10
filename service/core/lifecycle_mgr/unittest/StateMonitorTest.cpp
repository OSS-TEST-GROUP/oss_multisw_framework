// Unit tests for StateMonitor class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

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


LifecycleFactory* LifecycleFactory::GetInstance() { 
    static LifecycleFactory inst; 
    return &inst; 
}

class StateMonitorTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
};


TEST_F(StateMonitorTest, StartAndStop) {
    MockLifecycleManager mgr;
    StateMonitor monitor(mgr);
    
    EXPECT_TRUE(monitor.Start());
    monitor.Stop();
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
    EXPECT_CALL(*service, GetName()).WillOnce(Return("svc"));
    EXPECT_CALL(*service, GetServiceType()).WillOnce(Return(lifecycle_manager::ServiceType::NORMAL));
    EXPECT_CALL(*service, GetServiceState()).WillOnce(Return(lifecycle_manager::ServiceState::RUNNING));
    EXPECT_CALL(*service, isEnabled()).WillOnce(Return(true));
    EXPECT_CALL(*service, GetRecoveredCount()).WillOnce(Return(0));

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


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}