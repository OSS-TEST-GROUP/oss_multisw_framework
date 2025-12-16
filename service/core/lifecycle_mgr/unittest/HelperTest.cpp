// Unit tests for Helper and TypeConvert classes using Google Test
#include <gtest/gtest.h>

#include "include/Helper.hpp"
//#include "include/TypeConvert.hpp"
//#include "lifecycle/LifecycleCommon.hpp"


using namespace lifecycle_manager;

class HelperTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test PubSubTypeStr conversion
TEST_F(HelperTest, PubSubTypeStr_None) {
    auto result = Helper::PubSubTypeStr(lifecycle_manager::PubSubTYpe::PUBSUB_NONE);
    EXPECT_EQ(result, "None");
}

TEST_F(HelperTest, PubSubTypeStr_PublishOnly) {
    auto result = Helper::PubSubTypeStr(lifecycle_manager::PubSubTYpe::PUBLISH_ONLY);
    EXPECT_EQ(result, "Publish");
}

TEST_F(HelperTest, PubSubTypeStr_SubscribeOnly) {
    auto result = Helper::PubSubTypeStr(lifecycle_manager::PubSubTYpe::SUBSCRIBE_ONLY);
    EXPECT_EQ(result, "Subscriber");
}

TEST_F(HelperTest, PubSubTypeStr_PubSubBoth) {
    auto result = Helper::PubSubTypeStr(lifecycle_manager::PubSubTYpe::PUBSUB_BOTH);
    EXPECT_EQ(result, "PubAndSub");
}

// Test RecoveryStr conversion
TEST_F(HelperTest, RecoveryStr_Default) {
    auto result = Helper::RecoveryStr(lifecycle_manager::RecoveryPolicy::R_DEFAULT);
    EXPECT_EQ(result, "Default");
}

TEST_F(HelperTest, RecoveryStr_Always) {
    auto result = Helper::RecoveryStr(lifecycle_manager::RecoveryPolicy::ALWAYS);
    EXPECT_EQ(result, "Always");
}

TEST_F(HelperTest, RecoveryStr_OnFailure) {
    auto result = Helper::RecoveryStr(lifecycle_manager::RecoveryPolicy::ON_FAILURE);
    EXPECT_EQ(result, "OnFailure");
}

TEST_F(HelperTest, RecoveryStr_OnSuccess) {
    auto result = Helper::RecoveryStr(lifecycle_manager::RecoveryPolicy::ON_SUCCESS);
    EXPECT_EQ(result, "OnSuccess");
}

TEST_F(HelperTest, RecoveryStr_OnNotResponding) {
    auto result = Helper::RecoveryStr(lifecycle_manager::RecoveryPolicy::ON_NOT_RESPONDING);
    EXPECT_EQ(result, "OnNotRespond");
}

TEST_F(HelperTest, RecoveryStr_Manual) {
    auto result = Helper::RecoveryStr(lifecycle_manager::RecoveryPolicy::MANUAL);
    EXPECT_EQ(result, "Manual");
}

// Test ActionStr conversion
TEST_F(HelperTest, ActionStr_None) {
    auto result = Helper::ActionStr(lifecycle_manager::ServiceControlAction::NONE);
    EXPECT_EQ(result, "None");
}

TEST_F(HelperTest, ActionStr_Start) {
    auto result = Helper::ActionStr(lifecycle_manager::ServiceControlAction::START);
    EXPECT_EQ(result, "Start");
}

TEST_F(HelperTest, ActionStr_Stop) {
    auto result = Helper::ActionStr(lifecycle_manager::ServiceControlAction::STOP);
    EXPECT_EQ(result, "Stop");
}

// Test SvcModeStr conversion
TEST_F(HelperTest, SvcModeStr_Normal) {
    auto result = Helper::SvcModeStr(lifecycle_manager::ServiceMode::NORMAL);
    EXPECT_EQ(result, "Normal");
}

TEST_F(HelperTest, SvcModeStr_OtaMode) {
    auto result = Helper::SvcModeStr(lifecycle_manager::ServiceMode::OTA_MODE);
    EXPECT_EQ(result, "OTA-Mode");
}

// Test StateStr conversion
TEST_F(HelperTest, StateStr_NotRunning) {
    auto result = Helper::StateStr(lifecycle_manager::ServiceState::NOT_RUNNING);
    EXPECT_EQ(result, "NotRunning");
}

TEST_F(HelperTest, StateStr_Initializing) {
    auto result = Helper::StateStr(lifecycle_manager::ServiceState::INITIALIZING);
    EXPECT_EQ(result, "Initializing");
}

TEST_F(HelperTest, StateStr_Running) {
    auto result = Helper::StateStr(lifecycle_manager::ServiceState::RUNNING);
    EXPECT_EQ(result, "Running");
}

TEST_F(HelperTest, StateStr_RunningNoResponse) {
    auto result = Helper::StateStr(lifecycle_manager::ServiceState::RUNNING_NO_RESPONSE);
    EXPECT_EQ(result, "NoResponse");
}

TEST_F(HelperTest, StateStr_Terminating) {
    auto result = Helper::StateStr(lifecycle_manager::ServiceState::TERMINATING);
    EXPECT_EQ(result, "Terminating");
}

// Test SvcTypeStr conversion
TEST_F(HelperTest, SvcTypeStr_Normal) {
    auto result = Helper::SvcTypeStr(lifecycle_manager::ServiceType::NORMAL);
    EXPECT_EQ(result, "Normal");
}

TEST_F(HelperTest, SvcTypeStr_OneShot) {
    auto result = Helper::SvcTypeStr(lifecycle_manager::ServiceType::ONESHOT);
    EXPECT_EQ(result, "OneShot");
}

// Test ManagerStateStr conversion
TEST_F(HelperTest, ManagerStateStr_Init) {
    auto result = Helper::ManagerStateStr(lifecycle_manager::ManagerState::MANAGER_INIT);
    EXPECT_EQ(result, "INIT");
}

TEST_F(HelperTest, ManagerStateStr_Started) {
    auto result = Helper::ManagerStateStr(lifecycle_manager::ManagerState::MANAGER_STARTED);
    EXPECT_EQ(result, "STARTED");
}

TEST_F(HelperTest, ManagerStateStr_Running) {
    auto result = Helper::ManagerStateStr(lifecycle_manager::ManagerState::MANAGER_RUNNING);
    EXPECT_EQ(result, "RUNNING");
}

TEST_F(HelperTest, ManagerStateStr_Pause) {
    auto result = Helper::ManagerStateStr(lifecycle_manager::ManagerState::MANAGER_PASUE);
    EXPECT_EQ(result, "PAUSE");
}

TEST_F(HelperTest, ManagerStateStr_Terminate) {
    auto result = Helper::ManagerStateStr(lifecycle_manager::ManagerState::MANAGER_TERMINATE);
    EXPECT_EQ(result, "TERMINATE");
}

// ===== TypeConvert Tests =====
#if 0
// Test ToServiceState (used in real code: StateMonitor.cpp, ServiceRegistry.cpp)
TEST_F(HelperTest, TypeConvert_ToServiceState_NotRunning) {
    // This conversion works because it's called with the fully qualified lifecyclemgr:: type
    auto result = TypeConvert::ToServiceState(lifecyclemgr::ServiceState::NOT_RUNNING);
    EXPECT_EQ(result, lifecycle_manager::ServiceState::NOT_RUNNING);
}

TEST_F(HelperTest, TypeConvert_ToServiceState_Initializing) {
    auto result = TypeConvert::ToServiceState(lifecyclemgr::ServiceState::INITIALIZING);
    EXPECT_EQ(result, lifecycle_manager::ServiceState::INITIALIZING);
}

TEST_F(HelperTest, TypeConvert_ToServiceState_Running) {
    auto result = TypeConvert::ToServiceState(lifecyclemgr::ServiceState::RUNNING);
    EXPECT_EQ(result, lifecycle_manager::ServiceState::RUNNING);
}

TEST_F(HelperTest, TypeConvert_ToServiceState_RunningNoResponse) {
    auto result = TypeConvert::ToServiceState(lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE);
    EXPECT_EQ(result, lifecycle_manager::ServiceState::RUNNING_NO_RESPONSE);
}

TEST_F(HelperTest, TypeConvert_ToServiceState_Terminating) {
    auto result = TypeConvert::ToServiceState(lifecyclemgr::ServiceState::TERMINATING);
    EXPECT_EQ(result, lifecycle_manager::ServiceState::TERMINATING);
}

// Test ToServiceMode (used in real code)
TEST_F(HelperTest, TypeConvert_ToServiceMode_Normal) {
    auto result = TypeConvert::ToServiceMode(lifecyclemgr::ServiceMode::NORMAL);
    EXPECT_EQ(result, lifecycle_manager::ServiceMode::NORMAL);
}

TEST_F(HelperTest, TypeConvert_ToServiceMode_OtaMode) {
    auto result = TypeConvert::ToServiceMode(lifecyclemgr::ServiceMode::OTA_MODE);
    EXPECT_EQ(result, lifecycle_manager::ServiceMode::OTA_MODE);
}

// Test ToServiceType (used in real code)
TEST_F(HelperTest, TypeConvert_ToServiceType_Normal) {
    auto result = TypeConvert::ToServiceType(lifecyclemgr::ServiceType::NORMAL);
    EXPECT_EQ(result, lifecycle_manager::ServiceType::NORMAL);
}

TEST_F(HelperTest, TypeConvert_ToServiceType_OneShot) {
    auto result = TypeConvert::ToServiceType(lifecyclemgr::ServiceType::ONESHOT);
    EXPECT_EQ(result, lifecycle_manager::ServiceType::ONESHOT);
}

// Test ToRecoveryPolicy
TEST_F(HelperTest, TypeConvert_ToRecoveryPolicy_Default) {
    auto result = TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::R_DEFAULT);
    EXPECT_EQ(result, lifecycle_manager::RecoveryPolicy::R_DEFAULT);
}

TEST_F(HelperTest, TypeConvert_ToRecoveryPolicy_Always) {
    auto result = TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ALWAYS);
    EXPECT_EQ(result, lifecycle_manager::RecoveryPolicy::ALWAYS);
}

TEST_F(HelperTest, TypeConvert_ToRecoveryPolicy_OnFailure) {
    auto result = TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ON_FAILURE);
    EXPECT_EQ(result, lifecycle_manager::RecoveryPolicy::ON_FAILURE);
}

TEST_F(HelperTest, TypeConvert_ToRecoveryPolicy_OnSuccess) {
    auto result = TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ON_SUCCESS);
    EXPECT_EQ(result, lifecycle_manager::RecoveryPolicy::ON_SUCCESS);
}

TEST_F(HelperTest, TypeConvert_ToRecoveryPolicy_OnNotResponding) {
    auto result = TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::ON_NOT_RESPONDING);
    EXPECT_EQ(result, lifecycle_manager::RecoveryPolicy::ON_NOT_RESPONDING);
}

TEST_F(HelperTest, TypeConvert_ToRecoveryPolicy_Manual) {
    auto result = TypeConvert::ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy::MANUAL);
    EXPECT_EQ(result, lifecycle_manager::RecoveryPolicy::MANUAL);
}

// Test ToOsType
TEST_F(HelperTest, TypeConvert_ToOsType_ROS2) {
    auto result = TypeConvert::ToOsType(lifecyclemgr::OsType::ROS2);
    EXPECT_EQ(result, lifecycle_manager::OsType::ROS2);
}

TEST_F(HelperTest, TypeConvert_ToOsType_AGL) {
    auto result = TypeConvert::ToOsType(lifecyclemgr::OsType::AGL);
    EXPECT_EQ(result, lifecycle_manager::OsType::AGL);
}

TEST_F(HelperTest, TypeConvert_ToOsType_AUTOSAR) {
    auto result = TypeConvert::ToOsType(lifecyclemgr::OsType::AUTOSAR);
    EXPECT_EQ(result, lifecycle_manager::OsType::AUTOSAR);
}
#endif
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
