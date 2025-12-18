// Unit tests for TypeConvert class
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include implementation which brings in all necessary dependencies
#include "service/Service.cpp"
#include "service/TypeConvert.hpp"
#include "mock/MockLogger.hpp"

using namespace ros2::adsw_manager;
using ::testing::Return;
using ::testing::_;

class TypeConvertTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
           
    }

    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
};

// ============================================================================
// ServiceType conversion tests
// ============================================================================

TEST_F(TypeConvertTest, ToServiceType_FromLifecyclemgr_Normal) {
    EXPECT_EQ(TypeConvert::ToServiceType(lifecyclemgr::ServiceType::NORMAL), ServiceType::NORMAL);
}

TEST_F(TypeConvertTest, ToServiceType_FromLifecyclemgr_Oneshot) {
    EXPECT_EQ(TypeConvert::ToServiceType(lifecyclemgr::ServiceType::ONESHOT), ServiceType::ONESHOT);
}

TEST_F(TypeConvertTest, ToServiceType_FromLifecyclemgr_Default) {
    // Test default case with invalid value
    auto result = TypeConvert::ToServiceType(static_cast<lifecyclemgr::ServiceType>(999));
    EXPECT_EQ(result, ServiceType::NORMAL);
}

TEST_F(TypeConvertTest, ToServiceType_ToLifecyclemgr_Normal) {
    EXPECT_EQ(TypeConvert::ToServiceType(ServiceType::NORMAL), lifecyclemgr::ServiceType::NORMAL);
}

TEST_F(TypeConvertTest, ToServiceType_ToLifecyclemgr_Oneshot) {
    EXPECT_EQ(TypeConvert::ToServiceType(ServiceType::ONESHOT), lifecyclemgr::ServiceType::ONESHOT);
}

TEST_F(TypeConvertTest, ToServiceType_ToLifecyclemgr_Default) {
    // Test default case with invalid value
    auto result = TypeConvert::ToServiceType(static_cast<ServiceType>(999));
    EXPECT_EQ(result, lifecyclemgr::ServiceType::NORMAL);
}

// ============================================================================
// ServiceState conversion tests
// ============================================================================

TEST_F(TypeConvertTest, ToServiceState_FromLifecyclemgr_NotRunning) {
    EXPECT_EQ(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::NOT_RUNNING), ServiceState::NOT_RUNNING);
}

TEST_F(TypeConvertTest, ToServiceState_FromLifecyclemgr_Initializing) {
    EXPECT_EQ(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::INITIALIZING), ServiceState::INITIALIZING);
}

TEST_F(TypeConvertTest, ToServiceState_FromLifecyclemgr_Running) {
    EXPECT_EQ(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::RUNNING), ServiceState::RUNNING);
}

TEST_F(TypeConvertTest, ToServiceState_FromLifecyclemgr_RunningNoResponse) {
    EXPECT_EQ(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE), ServiceState::RUNNING_NO_RESPONSE);
}

TEST_F(TypeConvertTest, ToServiceState_FromLifecyclemgr_Terminating) {
    EXPECT_EQ(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::TERMINATING), ServiceState::TERMINATING);
}

TEST_F(TypeConvertTest, ToServiceState_FromLifecyclemgr_Default) {
    auto result = TypeConvert::ToServiceState(static_cast<lifecyclemgr::ServiceState>(999));
    EXPECT_EQ(result, ServiceState::NOT_RUNNING);
}

TEST_F(TypeConvertTest, ToServiceState_ToLifecyclemgr_NotRunning) {
    EXPECT_EQ(TypeConvert::ToServiceState(ServiceState::NOT_RUNNING), lifecyclemgr::ServiceState::NOT_RUNNING);
}

TEST_F(TypeConvertTest, ToServiceState_ToLifecyclemgr_Initializing) {
    EXPECT_EQ(TypeConvert::ToServiceState(ServiceState::INITIALIZING), lifecyclemgr::ServiceState::INITIALIZING);
}

TEST_F(TypeConvertTest, ToServiceState_ToLifecyclemgr_Running) {
    EXPECT_EQ(TypeConvert::ToServiceState(ServiceState::RUNNING), lifecyclemgr::ServiceState::RUNNING);
}

TEST_F(TypeConvertTest, ToServiceState_ToLifecyclemgr_RunningNoResponse) {
    EXPECT_EQ(TypeConvert::ToServiceState(ServiceState::RUNNING_NO_RESPONSE), lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE);
}

TEST_F(TypeConvertTest, ToServiceState_ToLifecyclemgr_Terminating) {
    EXPECT_EQ(TypeConvert::ToServiceState(ServiceState::TERMINATING), lifecyclemgr::ServiceState::TERMINATING);
}

TEST_F(TypeConvertTest, ToServiceState_ToLifecyclemgr_Default) {
    auto result = TypeConvert::ToServiceState(static_cast<ServiceState>(999));
    EXPECT_EQ(result, lifecyclemgr::ServiceState::NOT_RUNNING);
}

// ============================================================================
// RecoveryPolicy conversion tests
// ============================================================================

TEST_F(TypeConvertTest, ToRecoveryPolicy_FromLifecyclemgr_Default) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::R_DEFAULT), RecoveryPolicy::R_DEFAULT);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_FromLifecyclemgr_Always) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ALWAYS), RecoveryPolicy::ALWAYS);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_FromLifecyclemgr_OnFailure) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ON_FAILURE), RecoveryPolicy::ON_FAILURE);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_FromLifecyclemgr_OnSuccess) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ON_SUCCESS), RecoveryPolicy::ON_SUCCESS);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_FromLifecyclemgr_OnNotResponding) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ON_NOT_RESPONDING), RecoveryPolicy::ON_NOT_RESPONDING);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_FromLifecyclemgr_Manual) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::MANUAL), RecoveryPolicy::MANUAL);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_FromLifecyclemgr_InvalidDefault) {
    auto result = TypeConvert::ToRecoveryPolicy(static_cast<lifecyclemgr::RecoveryPolicy>(999));
    EXPECT_EQ(result, RecoveryPolicy::R_DEFAULT);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_ToLifecyclemgr_Default) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(RecoveryPolicy::R_DEFAULT), lifecyclemgr::RecoveryPolicy::R_DEFAULT);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_ToLifecyclemgr_Always) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(RecoveryPolicy::ALWAYS), lifecyclemgr::RecoveryPolicy::ALWAYS);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_ToLifecyclemgr_OnFailure) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(RecoveryPolicy::ON_FAILURE), lifecyclemgr::RecoveryPolicy::ON_FAILURE);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_ToLifecyclemgr_OnSuccess) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(RecoveryPolicy::ON_SUCCESS), lifecyclemgr::RecoveryPolicy::ON_SUCCESS);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_ToLifecyclemgr_OnNotResponding) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(RecoveryPolicy::ON_NOT_RESPONDING), lifecyclemgr::RecoveryPolicy::ON_NOT_RESPONDING);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_ToLifecyclemgr_Manual) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(RecoveryPolicy::MANUAL), lifecyclemgr::RecoveryPolicy::MANUAL);
}

TEST_F(TypeConvertTest, ToRecoveryPolicy_ToLifecyclemgr_InvalidDefault) {
    auto result = TypeConvert::ToRecoveryPolicy(static_cast<RecoveryPolicy>(999));
    EXPECT_EQ(result, lifecyclemgr::RecoveryPolicy::R_DEFAULT);
}

// ============================================================================
// OsType conversion tests
// ============================================================================

TEST_F(TypeConvertTest, ToOsType_FromLifecyclemgr_ROS2) {
    EXPECT_EQ(TypeConvert::ToOsType(lifecyclemgr::OsType::ROS2), OsType::ROS2);
}

TEST_F(TypeConvertTest, ToOsType_FromLifecyclemgr_AGL) {
    EXPECT_EQ(TypeConvert::ToOsType(lifecyclemgr::OsType::AGL), OsType::AGL);
}

TEST_F(TypeConvertTest, ToOsType_FromLifecyclemgr_AUTOSAR) {
    EXPECT_EQ(TypeConvert::ToOsType(lifecyclemgr::OsType::AUTOSAR), OsType::AUTOSAR);
}

TEST_F(TypeConvertTest, ToOsType_ToLifecyclemgr_ROS2) {
    EXPECT_EQ(TypeConvert::ToOsType(OsType::ROS2), lifecyclemgr::OsType::ROS2);
}

TEST_F(TypeConvertTest, ToOsType_ToLifecyclemgr_AGL) {
    EXPECT_EQ(TypeConvert::ToOsType(OsType::AGL), lifecyclemgr::OsType::AGL);
}

TEST_F(TypeConvertTest, ToOsType_ToLifecyclemgr_AUTOSAR) {
    EXPECT_EQ(TypeConvert::ToOsType(OsType::AUTOSAR), lifecyclemgr::OsType::AUTOSAR);
}

// ============================================================================
// Roundtrip tests (convert and convert back)
// ============================================================================

TEST_F(TypeConvertTest, ServiceType_Roundtrip) {
    // lifecyclemgr -> internal -> lifecyclemgr
    EXPECT_EQ(TypeConvert::ToServiceType(TypeConvert::ToServiceType(lifecyclemgr::ServiceType::NORMAL)), 
              lifecyclemgr::ServiceType::NORMAL);
    EXPECT_EQ(TypeConvert::ToServiceType(TypeConvert::ToServiceType(lifecyclemgr::ServiceType::ONESHOT)), 
              lifecyclemgr::ServiceType::ONESHOT);
}

TEST_F(TypeConvertTest, ServiceState_Roundtrip) {
    EXPECT_EQ(TypeConvert::ToServiceState(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::NOT_RUNNING)), 
              lifecyclemgr::ServiceState::NOT_RUNNING);
    EXPECT_EQ(TypeConvert::ToServiceState(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::RUNNING)), 
              lifecyclemgr::ServiceState::RUNNING);
    EXPECT_EQ(TypeConvert::ToServiceState(TypeConvert::ToServiceState(lifecyclemgr::ServiceState::INITIALIZING)), 
              lifecyclemgr::ServiceState::INITIALIZING);
}

TEST_F(TypeConvertTest, RecoveryPolicy_Roundtrip) {
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ALWAYS)), 
              lifecyclemgr::RecoveryPolicy::ALWAYS);
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::MANUAL)), 
              lifecyclemgr::RecoveryPolicy::MANUAL);
}

TEST_F(TypeConvertTest, OsType_Roundtrip) {
    EXPECT_EQ(TypeConvert::ToOsType(TypeConvert::ToOsType(lifecyclemgr::OsType::ROS2)), 
              lifecyclemgr::OsType::ROS2);
    EXPECT_EQ(TypeConvert::ToOsType(TypeConvert::ToOsType(lifecyclemgr::OsType::AGL)), 
              lifecyclemgr::OsType::AGL);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
