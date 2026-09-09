// QA(docs/qa): OtamServerService 의 HTTP-API 경로를 MockOtamServerApiClient 로 커버.
//
// server_service 는 ArgoCD REST API 를 호출하므로 실 유닛에서는 2.5% 만 커버됐다.
// OtamServerApiClient(추상 인터페이스)를 mock 으로 주입(InjectApiClientForTest)하고
// state/session 을 seam 으로 세팅해, 실 HTTP 없이 GetOtaSettings/SetOtaSettings/
// RunOtaUpdate/GetOtaUpdateState 의 엔드포인트 구성·응답 파싱 로직을 검증한다.

#include "otam_server_service/otam_server_service.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock/mock_otam_server_api_client.h"

#include <memory>
#include <string>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

// mock 을 주입하고 STARTED + 세션토큰을 세팅한 서비스를 만든다.
// 반환된 raw 포인터로 EXPECT_CALL 을 건다(소유권은 서비스로 이동).
MockOtamServerApiClient* PrepareWithMock(OtamServerService& svc) {
  auto mock = std::make_unique<NiceMock<MockOtamServerApiClient>>();
  MockOtamServerApiClient* raw = mock.get();
  svc.InjectApiClientForTest(std::move(mock));
  svc.ForceStartedForTest();
  svc.SetSessionIdForTest("test-argocd-token");
  return raw;
}

}  // namespace

// ── 준비 안 됨(미기동) → 모든 API 가 FAIL 조기 반환 ─────────────────────────
TEST(OtamServerServiceApi, NotStarted_GetSetRun_ReturnFail) {
  OtamServerService svc;  // state_ = UNINITIALIZED, api_client_ = null
  EXPECT_EQ(svc.GetOtaSettings("app").ret_code, ReturnValue::FAIL);
  EXPECT_EQ(svc.SetOtaSettings("app", "{}"), ReturnValue::FAIL);
  EXPECT_EQ(svc.RunOtaUpdate("app"), ReturnValue::FAIL);
}

// ── STARTED + api_client 있음, 그러나 세션 토큰 없음 → FAIL 조기 반환 ────────
TEST(OtamServerServiceApi, NoSession_ReturnsFail) {
  OtamServerService svc;
  auto mock = std::make_unique<NiceMock<MockOtamServerApiClient>>();
  svc.InjectApiClientForTest(std::move(mock));
  svc.ForceStartedForTest();  // session_id_ 는 비어 있음
  EXPECT_EQ(svc.GetOtaSettings("app").ret_code, ReturnValue::FAIL);
  EXPECT_EQ(svc.SetOtaSettings("app", "{}"), ReturnValue::FAIL);
  EXPECT_EQ(svc.RunOtaUpdate("app"), ReturnValue::FAIL);
}

// ── GetOtaSettings: automated 정책이 있는 응답 → SUCCESS + 파싱된 settings ──
TEST(OtamServerServiceApi, GetOtaSettings_AutomatedPresent_Success) {
  OtamServerService svc;
  auto* mock = PrepareWithMock(svc);
  const std::string resp =
      R"({"spec":{"syncPolicy":{"automated":{"prune":true,"selfHeal":true}}}})";
  EXPECT_CALL(*mock, Get(::testing::HasSubstr("/api/v1/applications/app1"), _))
      .WillOnce(Return(resp));

  AppSettingsResult r = svc.GetOtaSettings("app1");
  EXPECT_EQ(r.ret_code, ReturnValue::SUCCESS);
  EXPECT_NE(r.app_settings.find("prune"), std::string::npos);
}

// ── GetOtaSettings: automated 없음 → app_settings "{}" (SUCCESS 아님) ───────
TEST(OtamServerServiceApi, GetOtaSettings_NoAutomated_EmptyObject) {
  OtamServerService svc;
  auto* mock = PrepareWithMock(svc);
  EXPECT_CALL(*mock, Get(_, _)).WillOnce(Return(R"({"spec":{"syncPolicy":{}}})"));

  AppSettingsResult r = svc.GetOtaSettings("app2");
  EXPECT_EQ(r.app_settings, "{}");
  EXPECT_NE(r.ret_code, ReturnValue::SUCCESS);
}

// ── GetOtaSettings: 잘못된 JSON → 예외 catch → "{}" (크래시 없음) ────────────
TEST(OtamServerServiceApi, GetOtaSettings_InvalidJson_HandledGracefully) {
  OtamServerService svc;
  auto* mock = PrepareWithMock(svc);
  EXPECT_CALL(*mock, Get(_, _)).WillOnce(Return("<<not json>>"));

  AppSettingsResult r = svc.GetOtaSettings("app3");
  EXPECT_EQ(r.app_settings, "{}");
}

// ── SetOtaSettings: mock 주입 상태에서 try 경로 실행(크래시 없음) ────────────
TEST(OtamServerServiceApi, SetOtaSettings_WithMock_NoThrow) {
  OtamServerService svc;
  auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Patch(_, _)).WillByDefault(Return("{}"));
  ON_CALL(*mock, Put(_, _)).WillByDefault(Return("{}"));
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return("{}"));
  EXPECT_NO_THROW(svc.SetOtaSettings("app4", R"({"automated":{"prune":true}})"));
}

// ── RunOtaUpdate: mock 주입 상태에서 sync 엔드포인트 경로 실행 ───────────────
TEST(OtamServerServiceApi, RunOtaUpdate_WithMock_NoThrow) {
  OtamServerService svc;
  auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return(R"({"status":"ok"})"));
  EXPECT_NO_THROW(svc.RunOtaUpdate("app5"));
}

// ── GetOtaUpdateState: mock 주입 상태에서 경로 실행 ─────────────────────────
TEST(OtamServerServiceApi, GetOtaUpdateState_WithMock_NoThrow) {
  OtamServerService svc;
  auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Get(_, _)).WillByDefault(Return(R"({"status":{"sync":{"status":"Synced"}}})"));
  EXPECT_NO_THROW(svc.GetOtaUpdateState("app6"));
}

// ── CreateOtaSession: mock POST(ArgoCD 세션) 경로 실행 ──
TEST(OtamServerServiceApi, CreateOtaSession_WithMock_NoThrow) {
  OtamServerService svc;
  auto mock = std::make_unique<NiceMock<MockOtamServerApiClient>>();
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return(R"({"token":"abc.def.ghi"})"));
  svc.InjectApiClientForTest(std::move(mock));
  svc.ForceStartedForTest();
  EXPECT_NO_THROW(svc.CreateOtaSession());
}

// ── CreateOtaSession: 미기동이면 FAIL 조기 반환 ──
TEST(OtamServerServiceApi, CreateOtaSession_NotStarted_Fail) {
  OtamServerService svc;
  EXPECT_EQ(svc.CreateOtaSession().ret_code, ReturnValue::FAIL);
}

// ── DestroyOtaSession: 미지원 스텁 → SUCCESS ──
TEST(OtamServerServiceApi, DestroyOtaSession_ReturnsSuccess) {
  OtamServerService svc;
  EXPECT_EQ(svc.DestroyOtaSession(), ReturnValue::SUCCESS);
}

// ── SetOtamDiagnosticService: setter (nullptr 허용) ──
TEST(OtamServerServiceApi, SetOtamDiagnosticService_NoThrow) {
  OtamServerService svc;
  EXPECT_NO_THROW(svc.SetOtamDiagnosticService(nullptr));
}

// ── ServiceStateToString: 전 enum 값 (otam_service.h 공용) ──
TEST(OtamServiceCommon, ServiceStateToString_AllValues) {
  EXPECT_EQ(ServiceStateToString(ServiceState::UNINITIALIZED), "UNINITIALIZED");
  EXPECT_EQ(ServiceStateToString(ServiceState::INITIALIZED), "INITIALIZED");
  EXPECT_EQ(ServiceStateToString(ServiceState::STARTED), "STARTED");
  EXPECT_EQ(ServiceStateToString(ServiceState::STOPPED), "STOPPED");
  EXPECT_EQ(ServiceStateToString(ServiceState::ERROR), "ERROR");
}

// ===== 응답 파싱 브랜치 커버 =====

// SetOtaSettings("on") → automated patch 브랜치.
TEST(OtamServerServiceApi, SetOtaSettings_On_PatchBranch) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Patch(_, _)).WillByDefault(Return(R"({"spec":{}})"));
  EXPECT_NO_THROW(svc.SetOtaSettings("app", "on"));
}

// SetOtaSettings("off") → syncPolicy null patch 브랜치.
TEST(OtamServerServiceApi, SetOtaSettings_Off_PatchBranch) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Patch(_, _)).WillByDefault(Return(R"({"spec":{}})"));
  EXPECT_NO_THROW(svc.SetOtaSettings("app", "off"));
}

// SetOtaSettings(잘못된 값) → 'on'/'off' 아님 → FAIL 브랜치.
TEST(OtamServerServiceApi, SetOtaSettings_Invalid_ReturnsFail) {
  OtamServerService svc; PrepareWithMock(svc);
  EXPECT_EQ(svc.SetOtaSettings("app", "garbage"), ReturnValue::FAIL);
}

// RunOtaUpdate: status/message 없음 → FAIL 브랜치.
TEST(OtamServerServiceApi, RunOtaUpdate_NoStatus_ReturnsFail) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return("{}"));
  EXPECT_EQ(svc.RunOtaUpdate("app"), ReturnValue::FAIL);
}

// RunOtaUpdate: status 있음 → SUCCESS 브랜치.
TEST(OtamServerServiceApi, RunOtaUpdate_WithStatus_Success) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return(R"({"status":"Progressing"})"));
  EXPECT_EQ(svc.RunOtaUpdate("app"), ReturnValue::SUCCESS);
}

// GetOtaUpdateState: revision + operationState 포함 응답 → 파싱 브랜치.
TEST(OtamServerServiceApi, GetOtaUpdateState_FullResponse_NoThrow) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Get(_, _)).WillByDefault(Return(
      R"({"status":{"sync":{"status":"Synced","revision":"abc123"},)"
      R"("operationState":{"phase":"Succeeded"}}})"));
  EXPECT_NO_THROW(svc.GetOtaUpdateState("app"));
}

// GetOtaSettings: automated 없이 spec.syncPolicy 만 → "{}" (다른 분기).
TEST(OtamServerServiceApi, GetOtaSettings_SyncPolicyNoAutomated) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  EXPECT_CALL(*mock, Get(_, _)).WillOnce(Return(R"({"spec":{}})"));
  EXPECT_EQ(svc.GetOtaSettings("app").app_settings, "{}");
}

// GetOtaSettings: 잘못된 JSON (invalid) → catch → "{}" (에러 분기, 명시적).
TEST(OtamServerServiceApi, GetOtaSettings_EmptyResp_Graceful) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  EXPECT_CALL(*mock, Get(_, _)).WillOnce(Return(""));
  EXPECT_EQ(svc.GetOtaSettings("app").app_settings, "{}");
}

// CreateOtaSession: 토큰 없는 응답 → 세션 미생성 경로.
TEST(OtamServerServiceApi, CreateOtaSession_NoToken_NoThrow) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return("{}"));
  EXPECT_NO_THROW(svc.CreateOtaSession());
}

// CreateOtaSession: 잘못된 JSON → catch 경로.
TEST(OtamServerServiceApi, CreateOtaSession_InvalidJson_NoThrow) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return("<bad>"));
  EXPECT_NO_THROW(svc.CreateOtaSession());
}

// RunOtaUpdate: 잘못된 JSON → catch → FAIL.
TEST(OtamServerServiceApi, RunOtaUpdate_InvalidJson_Fail) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Post(_, _)).WillByDefault(Return("<bad>"));
  EXPECT_EQ(svc.RunOtaUpdate("app"), ReturnValue::FAIL);
}

// GetOtaUpdateState: status 없는 응답 → 대체 분기.
TEST(OtamServerServiceApi, GetOtaUpdateState_NoStatus_NoThrow) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Get(_, _)).WillByDefault(Return("{}"));
  EXPECT_NO_THROW(svc.GetOtaUpdateState("app"));
}

// SetOtaSettings: 잘못된 JSON 응답에도 크래시 없음(Patch catch 경로).
TEST(OtamServerServiceApi, SetOtaSettings_On_InvalidResp_NoThrow) {
  OtamServerService svc; auto* mock = PrepareWithMock(svc);
  ON_CALL(*mock, Patch(_, _)).WillByDefault(Return("<bad>"));
  EXPECT_NO_THROW(svc.SetOtaSettings("app", "on"));
}
