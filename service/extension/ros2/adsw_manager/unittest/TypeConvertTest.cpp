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
// MwType conversion tests
// ============================================================================

TEST_F(TypeConvertTest, ToMwType_FromLifecyclemgr_ROS2) {
    EXPECT_EQ(TypeConvert::ToMwType(lifecyclemgr::MwType::ROS2), MwType::ROS2);
}

TEST_F(TypeConvertTest, ToMwType_FromLifecyclemgr_AGL) {
    EXPECT_EQ(TypeConvert::ToMwType(lifecyclemgr::MwType::AGL), MwType::AGL);
}

TEST_F(TypeConvertTest, ToMwType_FromLifecyclemgr_AUTOSAR) {
    EXPECT_EQ(TypeConvert::ToMwType(lifecyclemgr::MwType::AUTOSAR), MwType::AUTOSAR);
}

TEST_F(TypeConvertTest, ToMwType_ToLifecyclemgr_ROS2) {
    EXPECT_EQ(TypeConvert::ToMwType(MwType::ROS2), lifecyclemgr::MwType::ROS2);
}

TEST_F(TypeConvertTest, ToMwType_ToLifecyclemgr_AGL) {
    EXPECT_EQ(TypeConvert::ToMwType(MwType::AGL), lifecyclemgr::MwType::AGL);
}

TEST_F(TypeConvertTest, ToMwType_ToLifecyclemgr_AUTOSAR) {
    EXPECT_EQ(TypeConvert::ToMwType(MwType::AUTOSAR), lifecyclemgr::MwType::AUTOSAR);
}

// MW_NATIVE: service/extension/general 이 관리하는 서비스 -- 미들웨어 프레임워크 없이
// 도는 순수 프로세스/컨테이너. 이 값이 없던 동안 general adsw_manager 는 자기 서비스를
// ROS2 로 보고했고, mw_type 은 라우팅 키라서 ros2 adsw_manager 와 같은 버스에 있으면
// 둘 다 같은 요청에 반응했다.
TEST_F(TypeConvertTest, ToMwType_FromLifecyclemgr_MW_NATIVE) {
    EXPECT_EQ(TypeConvert::ToMwType(lifecyclemgr::MwType::MW_NATIVE), MwType::MW_NATIVE);
}

TEST_F(TypeConvertTest, ToMwType_ToLifecyclemgr_MW_NATIVE) {
    EXPECT_EQ(TypeConvert::ToMwType(MwType::MW_NATIVE), lifecyclemgr::MwType::MW_NATIVE);
}

TEST_F(TypeConvertTest, ToMwType_FromLifecyclemgr_MW_UNKNOWN) {
    EXPECT_EQ(TypeConvert::ToMwType(lifecyclemgr::MwType::MW_UNKNOWN), MwType::MW_UNKNOWN);
}

TEST_F(TypeConvertTest, ToMwType_ToLifecyclemgr_MW_UNKNOWN) {
    EXPECT_EQ(TypeConvert::ToMwType(MwType::MW_UNKNOWN), lifecyclemgr::MwType::MW_UNKNOWN);
}

// ★ 열거자 밖의 값은 MW_UNKNOWN 으로 떨어져야 한다 -- ROS2 가 아니라.
//
// 이 함수에는 switch 뒤 return 이 **없었다**. 값 없이 함수를 벗어나는 UB 였고, IDL 이
// 열거자를 늘리는 순간(지금 이 커밋이 그렇다) 그 값을 받은 기존 빌드가 거기로 빠진다.
// 그리고 fallback 을 ROS2 로 두면 "모른다" 가 "라우팅 대상이다" 로 바뀐다 -- 이 값이
// 라우팅 키이므로 조용히 남의 요청을 집어가게 된다.
TEST_F(TypeConvertTest, ToMwType_UnknownEnumerator_FallsBackToUnknown) {
    const auto out_of_range = static_cast<lifecyclemgr::MwType>(9999);
    EXPECT_EQ(TypeConvert::ToMwType(out_of_range), MwType::MW_UNKNOWN);
    EXPECT_EQ(TypeConvert::ToMwType(static_cast<MwType>(9999)), lifecyclemgr::MwType::MW_UNKNOWN);
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

TEST_F(TypeConvertTest, MwType_Roundtrip) {
    // 열거자를 늘릴 때 여기도 늘릴 것. 빠뜨리면 새 값이 변환에서 조용히 뭉개져도
    // 이 테스트는 통과한다 -- mw_type 은 라우팅 키라 그 침묵의 대가가 크다.
    for (const auto mw : {lifecyclemgr::MwType::ROS2,
                          lifecyclemgr::MwType::AGL,
                          lifecyclemgr::MwType::AUTOSAR,
                          lifecyclemgr::MwType::MW_NATIVE,
                          lifecyclemgr::MwType::MW_UNKNOWN}) {
        EXPECT_EQ(TypeConvert::ToMwType(TypeConvert::ToMwType(mw)), mw)
            << "MwType roundtrip lost value " << static_cast<int>(mw);
    }
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
