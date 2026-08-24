// Unit tests for Helper and TypeConvert classes using Google Test
#include <gtest/gtest.h>

#include "include/Helper.hpp"


#include "mock/mock_service.hpp"

#include "include/TypeConvert.hpp"

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

// ===== 열거형 범위 밖 입력 =====
//
// 위 테스트들은 정의된 열거값만 넣는다. 그래서 일곱 함수 전부 switch 를
// 빠져나온 뒤의 `return "";` 이 실행 0회였다.
//
// 왜 중요한가: 이 문자열들은 전부 로그·진단 리포트에 그대로 실린다. 값이
// 범위를 벗어나는 건 DDS 로 들어온 정수를 캐스팅할 때 실제로 일어난다
// (구버전 발신자, 열거형 확장 후 혼재 배포). 그때 이 함수들이 던지거나
// 미정의 동작으로 가면 로깅 한 줄 때문에 프로세스가 죽는다. 빈 문자열로
// 조용히 떨어지는 게 계약이고, 그 계약을 고정한다.
//
// switch 의 default 라벨이 없어 컴파일러가 "모든 값을 다뤘다"고 보므로,
// 이 경로는 오직 범위 밖 값으로만 도달할 수 있다.

TEST_F(HelperTest, PubSubTypeStr_OutOfRangeIsEmpty) {
    EXPECT_EQ(Helper::PubSubTypeStr(static_cast<lifecycle_manager::PubSubTYpe>(99)), "");
}

TEST_F(HelperTest, RecoveryStr_OutOfRangeIsEmpty) {
    EXPECT_EQ(Helper::RecoveryStr(static_cast<lifecycle_manager::RecoveryPolicy>(99)), "");
}

TEST_F(HelperTest, ActionStr_OutOfRangeIsEmpty) {
    EXPECT_EQ(Helper::ActionStr(static_cast<lifecycle_manager::ServiceControlAction>(99)), "");
}

TEST_F(HelperTest, SvcModeStr_OutOfRangeIsEmpty) {
    EXPECT_EQ(Helper::SvcModeStr(static_cast<lifecycle_manager::ServiceMode>(99)), "");
}

TEST_F(HelperTest, StateStr_OutOfRangeIsEmpty) {
    EXPECT_EQ(Helper::StateStr(static_cast<lifecycle_manager::ServiceState>(99)), "");
}

TEST_F(HelperTest, SvcTypeStr_OutOfRangeIsEmpty) {
    EXPECT_EQ(Helper::SvcTypeStr(static_cast<lifecycle_manager::ServiceType>(99)), "");
}

TEST_F(HelperTest, ManagerStateStr_OutOfRangeIsEmpty) {
    EXPECT_EQ(Helper::ManagerStateStr(static_cast<lifecycle_manager::ManagerState>(99)), "");
}

TEST_F(HelperTest, AllStringifiersSurviveOutOfRange) {
    // 로깅 경로에서 불리므로 어느 하나라도 던지면 그 로그를 찍으려던
    // 코드 전체가 무너진다.
    EXPECT_NO_THROW({
        (void)Helper::PubSubTypeStr(static_cast<lifecycle_manager::PubSubTYpe>(-1));
        (void)Helper::RecoveryStr(static_cast<lifecycle_manager::RecoveryPolicy>(-1));
        (void)Helper::ActionStr(static_cast<lifecycle_manager::ServiceControlAction>(-1));
        (void)Helper::SvcModeStr(static_cast<lifecycle_manager::ServiceMode>(-1));
        (void)Helper::StateStr(static_cast<lifecycle_manager::ServiceState>(-1));
        (void)Helper::SvcTypeStr(static_cast<lifecycle_manager::ServiceType>(-1));
        (void)Helper::ManagerStateStr(static_cast<lifecycle_manager::ManagerState>(-1));
    });
}

// ===== TypeConvert Tests =====

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

// Test ToMwType
TEST_F(HelperTest, TypeConvert_ToMwType_ROS2) {
    auto result = TypeConvert::ToMwType(lifecyclemgr::MwType::ROS2);
    EXPECT_EQ(result, lifecycle_manager::MwType::ROS2);
}

TEST_F(HelperTest, TypeConvert_ToMwType_AGL) {
    auto result = TypeConvert::ToMwType(lifecyclemgr::MwType::AGL);
    EXPECT_EQ(result, lifecycle_manager::MwType::AGL);
}

TEST_F(HelperTest, TypeConvert_ToMwType_AUTOSAR) {
    auto result = TypeConvert::ToMwType(lifecyclemgr::MwType::AUTOSAR);
    EXPECT_EQ(result, lifecycle_manager::MwType::AUTOSAR);
}

// ═══════════════════════════════════════════════════════════════════════════
// TypeConvert 전수 왕복 검증
//
// 위 테스트들은 몇몇 값만 짚어 보고, ToContainerType / ToContainerState 는
// 아예 다루지 않는다. 변환기는 enum 하나가 늘어날 때 조용히 default 로 새는 게
// 전형적인 사고라, 모든 enum 값을 테이블로 돌려 왕복이 항등인지 확인한다.
// (86줄 중 43줄 미커버 — 순수 함수라 런타임 의존이 없다.)
// ═══════════════════════════════════════════════════════════════════════════

namespace {

using lifecycle_manager::TypeConvert;

// idl enum -> local enum -> idl enum 이 항등이어야 한다.
template <typename IdlEnum, typename LocalEnum, typename ToLocal, typename ToIdl>
void ExpectRoundTrip(const std::vector<std::pair<IdlEnum, LocalEnum>>& pairs,
                     ToLocal to_local, ToIdl to_idl) {
    for (const auto& p : pairs) {
        EXPECT_EQ(to_local(p.first), p.second);
        EXPECT_EQ(to_idl(p.second), p.first);
    }
}

} // namespace

TEST(TypeConvertTableTest, ServiceType_AllValuesRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::ServiceType, lifecycle_manager::ServiceType>(
        {{lifecyclemgr::ServiceType::NORMAL,  lifecycle_manager::ServiceType::NORMAL},
         {lifecyclemgr::ServiceType::ONESHOT, lifecycle_manager::ServiceType::ONESHOT}},
        [](lifecyclemgr::ServiceType v) { return TypeConvert::ToServiceType(v); },
        [](lifecycle_manager::ServiceType v) { return TypeConvert::ToServiceType(v); });
}

TEST(TypeConvertTableTest, ServiceState_AllValuesRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::ServiceState, lifecycle_manager::ServiceState>(
        {{lifecyclemgr::ServiceState::NOT_RUNNING,         lifecycle_manager::ServiceState::NOT_RUNNING},
         {lifecyclemgr::ServiceState::INITIALIZING,        lifecycle_manager::ServiceState::INITIALIZING},
         {lifecyclemgr::ServiceState::RUNNING,             lifecycle_manager::ServiceState::RUNNING},
         {lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE, lifecycle_manager::ServiceState::RUNNING_NO_RESPONSE},
         {lifecyclemgr::ServiceState::TERMINATING,         lifecycle_manager::ServiceState::TERMINATING}},
        [](lifecyclemgr::ServiceState v) { return TypeConvert::ToServiceState(v); },
        [](lifecycle_manager::ServiceState v) { return TypeConvert::ToServiceState(v); });
}

TEST(TypeConvertTableTest, ServiceMode_AllValuesRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::ServiceMode, lifecycle_manager::ServiceMode>(
        {{lifecyclemgr::ServiceMode::NORMAL,   lifecycle_manager::ServiceMode::NORMAL},
         {lifecyclemgr::ServiceMode::OTA_MODE, lifecycle_manager::ServiceMode::OTA_MODE}},
        [](lifecyclemgr::ServiceMode v) { return TypeConvert::ToServiceMode(v); },
        [](lifecycle_manager::ServiceMode v) { return TypeConvert::ToServiceMode(v); });
}

TEST(TypeConvertTableTest, RecoveryPolicy_AllValuesRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::RecoveryPolicy, lifecycle_manager::RecoveryPolicy>(
        {{lifecyclemgr::RecoveryPolicy::R_DEFAULT,         lifecycle_manager::RecoveryPolicy::R_DEFAULT},
         {lifecyclemgr::RecoveryPolicy::ALWAYS,            lifecycle_manager::RecoveryPolicy::ALWAYS},
         {lifecyclemgr::RecoveryPolicy::ON_FAILURE,        lifecycle_manager::RecoveryPolicy::ON_FAILURE},
         {lifecyclemgr::RecoveryPolicy::ON_SUCCESS,        lifecycle_manager::RecoveryPolicy::ON_SUCCESS},
         {lifecyclemgr::RecoveryPolicy::ON_NOT_RESPONDING, lifecycle_manager::RecoveryPolicy::ON_NOT_RESPONDING},
         {lifecyclemgr::RecoveryPolicy::MANUAL,            lifecycle_manager::RecoveryPolicy::MANUAL}},
        [](lifecyclemgr::RecoveryPolicy v) { return TypeConvert::ToRecoveryPolicy(v); },
        [](lifecycle_manager::RecoveryPolicy v) { return TypeConvert::ToRecoveryPolicy(v); });
}

TEST(TypeConvertTableTest, MwType_AllValuesRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::MwType, lifecycle_manager::MwType>(
        {{lifecyclemgr::MwType::ROS2,       lifecycle_manager::MwType::ROS2},
         {lifecyclemgr::MwType::AGL,        lifecycle_manager::MwType::AGL},
         {lifecyclemgr::MwType::AUTOSAR,    lifecycle_manager::MwType::AUTOSAR},
         {lifecyclemgr::MwType::MW_NATIVE,  lifecycle_manager::MwType::MW_NATIVE},
         {lifecyclemgr::MwType::MW_UNKNOWN, lifecycle_manager::MwType::MW_UNKNOWN}},
        [](lifecyclemgr::MwType v) { return TypeConvert::ToMwType(v); },
        [](lifecycle_manager::MwType v) { return TypeConvert::ToMwType(v); });
}

TEST(TypeConvertTableTest, ContainerType_AllValuesRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::ContainerType, lifecycle_manager::ContainerType>(
        {{lifecyclemgr::ContainerType::CT_UNKNOWN,     lifecycle_manager::ContainerType::CT_UNKNOWN},
         {lifecyclemgr::ContainerType::DOCKER,         lifecycle_manager::ContainerType::DOCKER},
         {lifecyclemgr::ContainerType::DOCKER_COMPOSE, lifecycle_manager::ContainerType::DOCKER_COMPOSE},
         {lifecyclemgr::ContainerType::K3S,            lifecycle_manager::ContainerType::K3S}},
        [](lifecyclemgr::ContainerType v) { return TypeConvert::ToContainerType(v); },
        [](lifecycle_manager::ContainerType v) { return TypeConvert::ToContainerType(v); });
}

TEST(TypeConvertTableTest, ContainerState_AllValuesRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::ContainerState, lifecycle_manager::ContainerState>(
        {{lifecyclemgr::ContainerState::CS_UNKNOWN,        lifecycle_manager::ContainerState::CS_UNKNOWN},
         {lifecyclemgr::ContainerState::CONTAINER_STOPPED, lifecycle_manager::ContainerState::CONTAINER_STOPPED},
         {lifecyclemgr::ContainerState::CONTAINER_RUNNING, lifecycle_manager::ContainerState::CONTAINER_RUNNING},
         {lifecyclemgr::ContainerState::CONTAINER_PAUSED,  lifecycle_manager::ContainerState::CONTAINER_PAUSED}},
        [](lifecyclemgr::ContainerState v) { return TypeConvert::ToContainerState(v); },
        [](lifecycle_manager::ContainerState v) { return TypeConvert::ToContainerState(v); });
}

// 범위를 벗어난 값은 각 변환기가 정한 안전한 기본값으로 떨어져야 한다.
// 특히 MwType 은 "모름"을 ROS2 로 접으면 안 된다 — 그러면 '모른다' 가
// '라우팅 대상이다' 로 바뀐다 (TypeConvert.hpp 주석 참조).
TEST(TypeConvertTableTest, OutOfRangeFallsBackToSafeDefault) {
    EXPECT_EQ(TypeConvert::ToMwType(static_cast<lifecyclemgr::MwType>(99)),
              lifecycle_manager::MwType::MW_UNKNOWN);
    EXPECT_EQ(TypeConvert::ToMwType(static_cast<lifecycle_manager::MwType>(99)),
              lifecyclemgr::MwType::MW_UNKNOWN);

    EXPECT_EQ(TypeConvert::ToServiceState(static_cast<lifecyclemgr::ServiceState>(99)),
              lifecycle_manager::ServiceState::NOT_RUNNING);
    EXPECT_EQ(TypeConvert::ToServiceType(static_cast<lifecyclemgr::ServiceType>(99)),
              lifecycle_manager::ServiceType::NORMAL);
    EXPECT_EQ(TypeConvert::ToServiceMode(static_cast<lifecyclemgr::ServiceMode>(99)),
              lifecycle_manager::ServiceMode::NORMAL);
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(static_cast<lifecyclemgr::RecoveryPolicy>(99)),
              lifecycle_manager::RecoveryPolicy::R_DEFAULT);
    EXPECT_EQ(TypeConvert::ToContainerType(static_cast<lifecyclemgr::ContainerType>(99)),
              lifecycle_manager::ContainerType::CT_UNKNOWN);
    EXPECT_EQ(TypeConvert::ToContainerState(static_cast<lifecyclemgr::ContainerState>(99)),
              lifecycle_manager::ContainerState::CS_UNKNOWN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
