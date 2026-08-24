// F013/F014 로 추가된 ctrl·container 엔드포인트 10종의 유닛 테스트.
//
// 왜 필요했나:
//   이 10개 파일은 테스트가 하나도 없었다. 커버리지가 라인 50%(10/20)로
//   찍혀 있었는데, 그건 EndPointFactory 가 생성자를 부른 덕이고
//   OnDataAvailable() / OnDataPublished() 는 실행 카운트가 0 이었다.
//
// 왜 실 DDS 를 쓰지 않나:
//   커버리지는 유닛 레인 실행분으로만 측정된다. ConcreteEndpointIntegrationTest
//   처럼 실 DDS 를 쓰는 테스트는 integration-tests.regex 로 유닛 레인에서
//   제외되므로 거기에 추가하면 측정 커버리지가 올라가지 않는다.
//
//   DDSMessageEndPoint::Read<T>() 는 reader_ 가 nullptr 이면 곧바로 false 를
//   돌려준다. 그래서 Initialize() 하지 않은 엔드포인트로도 함수 진입 · 로깅 ·
//   Read 가드까지는 그대로 밟을 수 있다. 콜백이 실제로 불리는 경로
//   (on_data_cb_(msg)) 만 실 리더가 필요한데, 그쪽은 통합 레인의
//   ConcreteEndpointIntegrationTest 가 같은 형태로 검증한다.
//
//   즉 여기서 고정하는 계약은 두 가지다:
//     - 읽을 데이터가 없으면 OnDataAvailable 은 콜백을 부르지 않고 조용히 끝난다
//     - OnDataPublished 는 성공/실패 양쪽에서 예외 없이 끝난다

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>

#include "endpoint/CtrlStartServiceEndPoint.hpp"
#include "endpoint/CtrlStopServiceEndPoint.hpp"
#include "endpoint/CtrlSetServiceModeEndPoint.hpp"
#include "endpoint/CtrlSetRecoveryPolicyEndPoint.hpp"
#include "endpoint/CtrlStartContainerEndPoint.hpp"
#include "endpoint/CtrlStopContainerEndPoint.hpp"
#include "endpoint/SetRecoveryPolicyEndPoint.hpp"
#include "endpoint/StartContainerEndPoint.hpp"
#include "endpoint/StopContainerEndPoint.hpp"
#include "endpoint/NotifyContainerStateEndPoint.hpp"

#include "mock/mock_logger.hpp"

using namespace lifecycle_manager;

namespace {

class CtrlEndPointTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Mock_Logger = new ::testing::NiceMock<MockLogger>();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(::testing::Return(Mock_Logger));
    }
    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
};

// 엔드포인트마다 같은 세 가지를 확인한다. 타입이 전부 달라 템플릿으로 묶는다.
//   1) 요청/응답 두 MessageType 으로 각각 생성된다 (생성자 if/else-if 양쪽)
//   2) 리더가 없으면 OnDataAvailable 이 콜백 없이 조용히 끝난다
//   3) OnDataPublished 가 성공/실패 양쪽에서 예외 없이 끝난다
template <typename EP>
void ExpectEndPointContract(MessageType req_type, MessageType resp_type,
                            const std::string& topic) {
    // (1) 두 타입 모두 생성 가능
    EP reader_side(req_type, topic);
    EP writer_side(resp_type, topic);

    // (2) Initialize() 를 하지 않았으므로 reader_ 는 nullptr —
    //     Read<T>() 가 false 를 돌려주고 콜백은 불리지 않아야 한다.
    bool cb_called = false;
    EP with_cb(req_type, topic, [&cb_called](auto*) { cb_called = true; });
    EXPECT_NO_THROW(with_cb.OnDataAvailable());
    EXPECT_FALSE(cb_called) << "리더가 없는데 콜백이 불렸다";

    // 콜백이 아예 없어도 안전해야 한다.
    EXPECT_NO_THROW(reader_side.OnDataAvailable());

    // (3) 발행 결과 로깅 — 성공/실패 양쪽
    EXPECT_NO_THROW(writer_side.OnDataPublished(true));
    EXPECT_NO_THROW(writer_side.OnDataPublished(false));
}

TEST_F(CtrlEndPointTest, CtrlStartService) {
    ExpectEndPointContract<CtrlStartServiceEndPoint>(
        MessageType::CTRL_START_SERVICE_REQUEST,
        MessageType::CTRL_START_SERVICE_RESPONSE,
        "mmf/lifecyclemgr/ctrl/StartServiceRequest");
}

TEST_F(CtrlEndPointTest, CtrlStopService) {
    ExpectEndPointContract<CtrlStopServiceEndPoint>(
        MessageType::CTRL_STOP_SERVICE_REQUEST,
        MessageType::CTRL_STOP_SERVICE_RESPONSE,
        "mmf/lifecyclemgr/ctrl/StopServiceRequest");
}

TEST_F(CtrlEndPointTest, CtrlSetServiceMode) {
    ExpectEndPointContract<CtrlSetServiceModeEndPoint>(
        MessageType::CTRL_SET_SERVICE_MODE_REQUEST,
        MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
        "mmf/lifecyclemgr/ctrl/SetServiceModeRequest");
}

TEST_F(CtrlEndPointTest, CtrlSetRecoveryPolicy) {
    ExpectEndPointContract<CtrlSetRecoveryPolicyEndPoint>(
        MessageType::CTRL_SET_RECOVERY_POLICY_REQUEST,
        MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE,
        "mmf/lifecyclemgr/ctrl/SetRecoveryPolicyRequest");
}

TEST_F(CtrlEndPointTest, CtrlStartContainer) {
    ExpectEndPointContract<CtrlStartContainerEndPoint>(
        MessageType::CTRL_START_CONTAINER_REQUEST,
        MessageType::CTRL_START_CONTAINER_RESPONSE,
        "mmf/lifecyclemgr/ctrl/StartContainerRequest");
}

TEST_F(CtrlEndPointTest, CtrlStopContainer) {
    ExpectEndPointContract<CtrlStopContainerEndPoint>(
        MessageType::CTRL_STOP_CONTAINER_REQUEST,
        MessageType::CTRL_STOP_CONTAINER_RESPONSE,
        "mmf/lifecyclemgr/ctrl/StopContainerRequest");
}

TEST_F(CtrlEndPointTest, SetRecoveryPolicy) {
    ExpectEndPointContract<SetRecoveryPolicyEndPoint>(
        MessageType::SET_RECOVERY_POLICY_REQUEST,
        MessageType::SET_RECOVERY_POLICY_RESPONSE,
        "mmf/lifecyclemgr/SetRecoveryPolicyRequest");
}

TEST_F(CtrlEndPointTest, StartContainer) {
    ExpectEndPointContract<StartContainerEndPoint>(
        MessageType::START_CONTAINER_REQUEST,
        MessageType::START_CONTAINER_RESPONSE,
        "mmf/lifecyclemgr/StartContainerRequest");
}

TEST_F(CtrlEndPointTest, StopContainer) {
    ExpectEndPointContract<StopContainerEndPoint>(
        MessageType::STOP_CONTAINER_REQUEST,
        MessageType::STOP_CONTAINER_RESPONSE,
        "mmf/lifecyclemgr/StopContainerRequest");
}

TEST_F(CtrlEndPointTest, NotifyContainerState) {
    ExpectEndPointContract<NotifyContainerStateEndPoint>(
        MessageType::CONTAINER_STATE_REPORT,
        MessageType::NOTIFY_CONTAINER_STATE,
        "mmf/lifecyclemgr/NotifyContainerState");
}

} // namespace

// 이 저장소의 유닛 테스트는 gtest_main 을 링크하지 않고 각자 main 을 둔다.
int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
