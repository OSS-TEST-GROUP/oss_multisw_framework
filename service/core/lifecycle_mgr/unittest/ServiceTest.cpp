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
            MwType::ROS2
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
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s.GetName(), "TestService");
    EXPECT_EQ(s.GetServiceType(), ServiceType::NORMAL);
    EXPECT_EQ(s.GetRecoveryPolicy(), RecoveryPolicy::ALWAYS);
    EXPECT_TRUE(s.isEnabled());
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetRecoveredCount(), 0);
    EXPECT_EQ(s.GetMwType(), MwType::ROS2);
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
    Service s("PrintService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false, MwType::ROS2);
    EXPECT_NO_THROW(s.Print());
}

TEST_F(ServiceTest, ServiceStateNoChangeWhenSameState) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    
    // Set to same state - should update last state
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
}

TEST_F(ServiceTest, MultipleStateTransitions) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    
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
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    
    s.SetServiceMode(ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
    
    // Only OTA_MODE exists in production; avoid DEBUG/SAFE
    s.SetServiceMode(ServiceMode::OTA_MODE);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::OTA_MODE);
}

TEST_F(ServiceTest, AllServiceTypes) {
    Service s1("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s1.GetServiceType(), ServiceType::NORMAL);
    
    Service s2("svc2", ServiceType::ONESHOT, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s2.GetServiceType(), ServiceType::ONESHOT);
    
    // Avoid CRITICAL/OPTIONAL as they are not in production enums
}

TEST_F(ServiceTest, AllRecoveryPolicies) {
    Service s1("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s1.GetRecoveryPolicy(), RecoveryPolicy::ALWAYS);
    
    Service s2("svc2", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true, MwType::ROS2);
    EXPECT_EQ(s2.GetRecoveryPolicy(), RecoveryPolicy::ON_FAILURE);
    
    Service s3("svc3", ServiceType::NORMAL, RecoveryPolicy::R_DEFAULT, true, MwType::ROS2);
    EXPECT_EQ(s3.GetRecoveryPolicy(), RecoveryPolicy::R_DEFAULT);
}

TEST_F(ServiceTest, AllMwTypes) {
    Service s1("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s1.GetMwType(), MwType::ROS2);
    
    Service s2("svc2", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::AGL);
    EXPECT_EQ(s2.GetMwType(), MwType::AGL);
    
    Service s3("svc3", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::AUTOSAR);
    EXPECT_EQ(s3.GetMwType(), MwType::AUTOSAR);
}

TEST_F(ServiceTest, DisabledService) {
    Service s("DisabledService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false, MwType::ROS2);
    EXPECT_FALSE(s.isEnabled());
}

TEST_F(ServiceTest, RecoveryCountLargeValues) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s.GetRecoveredCount(), 0);
    
    for (int i = 0; i < 100; i++) {
        s.IncreaseRecoveredCount();
    }
    EXPECT_EQ(s.GetRecoveredCount(), 100);
    
    s.ClearRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 0);
}

TEST_F(ServiceTest, EmptyServiceName) {
    Service s("", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s.GetName(), "");
}

TEST_F(ServiceTest, LongServiceName) {
    std::string long_name(500, 'a');
    Service s(long_name, ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s.GetName(), long_name);
}

TEST_F(ServiceTest, SpecialCharactersInName) {
    Service s("service-name_123.test", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s.GetName(), "service-name_123.test");
}

// Additional tests for branch coverage

// Test SetServiceState with same state (branch where state doesn't change)
TEST_F(ServiceTest, SetServiceState_NoChange) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    
    // First set to RUNNING
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    
    // Set to same state - should NOT update last state
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING); // Should remain NOT_RUNNING
}

// Test all valid ServiceState transitions
TEST_F(ServiceTest, AllServiceStateTransitions) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    
    // NOT_RUNNING -> INITIALIZING
    s.SetServiceState(ServiceState::INITIALIZING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::INITIALIZING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    
    // INITIALIZING -> RUNNING
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::INITIALIZING);
    
    // RUNNING -> TERMINATING
    s.SetServiceState(ServiceState::TERMINATING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::TERMINATING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::RUNNING);
    
    // TERMINATING -> NOT_RUNNING
    s.SetServiceState(ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::TERMINATING);
}

// Test Print for all ServiceType variations
TEST_F(ServiceTest, Print_AllServiceTypes) {
    Service s1("normal_svc", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_NO_THROW(s1.Print());
    
    Service s2("oneshot_svc", ServiceType::ONESHOT, RecoveryPolicy::ON_FAILURE, false, MwType::AGL);
    EXPECT_NO_THROW(s2.Print());
}

// Test Print for all RecoveryPolicy variations
TEST_F(ServiceTest, Print_AllRecoveryPolicies) {
    Service s1("always_svc", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_NO_THROW(s1.Print());
    
    Service s2("on_failure_svc", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true, MwType::ROS2);
    EXPECT_NO_THROW(s2.Print());
    
    Service s3("manual_svc", ServiceType::NORMAL, RecoveryPolicy::MANUAL, true, MwType::ROS2);
    EXPECT_NO_THROW(s3.Print());
    
    Service s4("on_success_svc", ServiceType::NORMAL, RecoveryPolicy::ON_SUCCESS, true, MwType::ROS2);
    EXPECT_NO_THROW(s4.Print());
    
    Service s5("not_respond_svc", ServiceType::NORMAL, RecoveryPolicy::ON_NOT_RESPONDING, true, MwType::ROS2);
    EXPECT_NO_THROW(s5.Print());
}

// Test GetLastServiceState initial value
TEST_F(ServiceTest, GetLastServiceState_InitialValue) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
}

// Test recovery count boundary values
TEST_F(ServiceTest, RecoveryCount_BoundaryValues) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    
    // Clear at 0 should still be 0
    s.ClearRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 0);
    
    // Single increment
    s.IncreaseRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 1);
    
    // Clear after increments
    s.IncreaseRecoveredCount();
    s.IncreaseRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 3);
    s.ClearRecoveredCount();
    EXPECT_EQ(s.GetRecoveredCount(), 0);
}

// Test service mode transitions
TEST_F(ServiceTest, ServiceMode_Transitions) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
    
    s.SetServiceMode(ServiceMode::OTA_MODE);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::OTA_MODE);
    
    s.SetServiceMode(ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
}

// Test with R_DEFAULT recovery policy
TEST_F(ServiceTest, RecoveryPolicy_Default) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::R_DEFAULT, true, MwType::ROS2);
    EXPECT_EQ(s.GetRecoveryPolicy(), RecoveryPolicy::R_DEFAULT);
}

// Test destructor doesn't crash
TEST_F(ServiceTest, Destructor_DoesNotCrash) {
    Service* s = new Service("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    s->SetServiceState(ServiceState::RUNNING);
    s->IncreaseRecoveredCount();
    EXPECT_NO_THROW(delete s);
}

// Test default constructor state
TEST_F(ServiceTest, DefaultConstructor_AllFields) {
    Service s;
    EXPECT_EQ(s.GetName(), "");
    EXPECT_EQ(s.GetServiceType(), ServiceType::ONESHOT);
    EXPECT_EQ(s.GetRecoveryPolicy(), RecoveryPolicy::R_DEFAULT);
    EXPECT_FALSE(s.isEnabled());
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetRecoveredCount(), 0);
    EXPECT_EQ(s.GetMwType(), MwType::ROS2);
}

// Test rapid state transitions
TEST_F(ServiceTest, RapidStateTransitions) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true, MwType::ROS2);
    
    for (int i = 0; i < 10; i++) {
        s.SetServiceState(ServiceState::RUNNING);
        s.SetServiceState(ServiceState::NOT_RUNNING);
    }
    
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::RUNNING);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
