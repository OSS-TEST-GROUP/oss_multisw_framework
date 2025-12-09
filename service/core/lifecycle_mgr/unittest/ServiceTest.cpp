// Unit tests for Service class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <memory>

#include "service/Service.hpp"
#include "unittest/mock/mock_logger.hpp"

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

class ServiceTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new Service(
            "test_arg",
            ServiceType::NORMAL,
            RecoveryPolicy::ALWAYS,
            true,
            OsType::ROS2
        );
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        delete ins;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    Service* ins;
};


TEST_F(ServiceTest, DefaultConstructor) {
    Service s;
    EXPECT_EQ(s.GetName(), "");
    EXPECT_FALSE(s.isEnabled());
}

TEST_F(ServiceTest, SetAndGetServiceMode) {
    Service s;
    s.SetServiceMode(ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
}

TEST_F(ServiceTest, SetAndGetServiceState) {
    Service s;
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
}

// Add more tests for Service as needed
TEST_F(ServiceTest, ParameterizedConstructor) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s.GetName(), "TestService");
    EXPECT_EQ(s.GetServiceType(), ServiceType::NORMAL);
    EXPECT_EQ(s.GetRecoveryPolicy(), RecoveryPolicy::ALWAYS);
    EXPECT_TRUE(s.isEnabled());
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetRecoveredCount(), 0);
    EXPECT_EQ(s.GetOsType(), OsType::ROS2);
}

TEST_F(ServiceTest, ServiceStateTransition) {
    Service s;
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
}

TEST_F(ServiceTest, RecoverdCountOperations) {
    Service s;
    EXPECT_EQ(s.GetRecoveredCount(), 0);
    s.IncreaseRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 1);
    s.IncreaseRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 2);
    s.ClearRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 0);
}

TEST_F(ServiceTest, PrintDoesNotThrow) {
    Service s("PrintService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false, OsType::ROS2);
    EXPECT_NO_THROW(s.Print());
}

TEST_F(ServiceTest, ServiceStateNoChangeWhenSameState) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    
    // Set to same state - should update last state
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
}

TEST_F(ServiceTest, MultipleStateTransitions) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    
    // Use existing states only
    s.SetServiceState(ServiceState::INITIALIZING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::INITIALIZING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::INITIALIZING);
    
    s.SetServiceState(ServiceState::TERMINATING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::TERMINATING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::RUNNING);
    
    s.SetServiceState(ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::TERMINATING);
}

TEST_F(ServiceTest, AllServiceModes) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    
    s.SetServiceMode(ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
    
    // Only OTA_MODE exists in production; avoid DEBUG/SAFE
    s.SetServiceMode(ServiceMode::OTA_MODE);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::OTA_MODE);
}

TEST_F(ServiceTest, AllServiceTypes) {
    Service s1("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s1.GetServiceType(), ServiceType::NORMAL);
    
    Service s2("svc2", ServiceType::ONESHOT, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s2.GetServiceType(), ServiceType::ONESHOT);
    
    // Avoid CRITICAL/OPTIONAL as they are not in production enums
}

TEST_F(ServiceTest, AllRecoveryPolicies) {
    Service s1("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s1.GetRecoveryPolicy(), RecoveryPolicy::ALWAYS);
    
    Service s2("svc2", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true, OsType::ROS2);
    EXPECT_EQ(s2.GetRecoveryPolicy(), RecoveryPolicy::ON_FAILURE);
    
    Service s3("svc3", ServiceType::NORMAL, RecoveryPolicy::R_DEFAULT, true, OsType::ROS2);
    EXPECT_EQ(s3.GetRecoveryPolicy(), RecoveryPolicy::R_DEFAULT);
}

TEST_F(ServiceTest, AllOsTypes) {
    Service s1("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s1.GetOsType(), OsType::ROS2);
    
    Service s2("svc2", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::AGL);
    EXPECT_EQ(s2.GetOsType(), OsType::AGL);
    
    Service s3("svc3", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::AUTOSAR);
    EXPECT_EQ(s3.GetOsType(), OsType::AUTOSAR);
}

TEST_F(ServiceTest, DisabledService) {
    Service s("DisabledService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false, OsType::ROS2);
    EXPECT_FALSE(s.isEnabled());
}

TEST_F(ServiceTest, RecoveryCountLargeValues) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s.GetRecoveredCount(), 0);
    
    for (int i = 0; i < 100; i++) {
        s.IncreaseRecoveredCount();
    }
    EXPECT_EQ(s.GetRecoveredCount(), 100);
    
    s.ClearRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 0);
}

TEST_F(ServiceTest, EmptyServiceName) {
    Service s("", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s.GetName(), "");
}

TEST_F(ServiceTest, LongServiceName) {
    std::string long_name(500, 'a');
    Service s(long_name, ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s.GetName(), long_name);
}

TEST_F(ServiceTest, SpecialCharactersInName) {
    Service s("service-name_123.test", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, OsType::ROS2);
    EXPECT_EQ(s.GetName(), "service-name_123.test");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
