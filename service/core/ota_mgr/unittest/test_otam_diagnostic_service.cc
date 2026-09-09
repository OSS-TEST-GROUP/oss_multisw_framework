// F007 — unit tests for OtamDiagnosticService heartbeat configuration.
//
// We deliberately stay above the DDS Initialize() boundary: the heartbeat
// timer / publisher requires a live DataWriter to actually emit messages,
// and standing one up requires a real FastDDS participant which is heavy
// for a host unit-test. These tests cover what we can without DDS:
//   1. The default heartbeat interval is 3 s when the env var is unset.
//   2. HEARTBEAT_INTERVAL_SEC=N (positive int) overrides the default.
//   3. Empty / non-numeric / non-positive env values fall back to 3 s.
//   4. heartbeat_seq() starts at 0 (no publishes yet).
//   5. PublishOtaMgrInfo() called pre-Initialize buffers without throwing
//      (regression guard for ac5 — heartbeat work must not break the
//      existing buffering behaviour).
//
// The real ros2-topic-echo verification of ac4 happens inside the
// integration test environment (F004 + F011).

#include "otam_diagnostic_service/otam_diagnostic_service.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock/mock_otam_dds_impl.h"

#include <cstdlib>
#include <memory>
#include <string>

namespace {

// Test-only RAII helper: sets HEARTBEAT_INTERVAL_SEC for the lifetime of
// the test, then restores whatever value was there before (typically
// unset → unset).
class HeartbeatEnv {
 public:
  explicit HeartbeatEnv(const char* value) {
    const char* prev = std::getenv(kKey);
    had_prev_ = (prev != nullptr);
    if (had_prev_) prev_ = prev;
    if (value == nullptr) {
      ::unsetenv(kKey);
    } else {
      ::setenv(kKey, value, /*overwrite=*/1);
    }
  }
  ~HeartbeatEnv() {
    if (had_prev_) {
      ::setenv(kKey, prev_.c_str(), /*overwrite=*/1);
    } else {
      ::unsetenv(kKey);
    }
  }

 private:
  static constexpr const char* kKey = "HEARTBEAT_INTERVAL_SEC";
  bool had_prev_ = false;
  std::string prev_;
};

}  // namespace

TEST(OtamDiagnosticServiceHeartbeat, DefaultsToThreeSecondsWhenEnvUnset) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  EXPECT_EQ(svc.heartbeat_interval().count(), 3);
}

TEST(OtamDiagnosticServiceHeartbeat, EnvOverridesIntervalWhenPositive) {
  HeartbeatEnv guard{"10"};
  OtamDiagnosticService svc;
  EXPECT_EQ(svc.heartbeat_interval().count(), 10);
}

TEST(OtamDiagnosticServiceHeartbeat, EnvFallsBackToDefaultWhenEmpty) {
  HeartbeatEnv guard{""};
  OtamDiagnosticService svc;
  EXPECT_EQ(svc.heartbeat_interval().count(), 3);
}

TEST(OtamDiagnosticServiceHeartbeat, EnvFallsBackToDefaultWhenNonNumeric) {
  HeartbeatEnv guard{"five"};
  OtamDiagnosticService svc;
  EXPECT_EQ(svc.heartbeat_interval().count(), 3);
}

TEST(OtamDiagnosticServiceHeartbeat, EnvFallsBackToDefaultWhenZeroOrNegative) {
  {
    HeartbeatEnv guard{"0"};
    OtamDiagnosticService svc;
    EXPECT_EQ(svc.heartbeat_interval().count(), 3);
  }
  {
    HeartbeatEnv guard{"-5"};
    OtamDiagnosticService svc;
    EXPECT_EQ(svc.heartbeat_interval().count(), 3);
  }
}

TEST(OtamDiagnosticServiceHeartbeat, SeqStartsAtZero) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  EXPECT_EQ(svc.heartbeat_seq(), 0);
}

TEST(OtamDiagnosticServiceHeartbeat,
     PublishOtaMgrInfoPreInitializeDoesNotThrow) {
  // Regression guard for ac5: heartbeat additions must not change the
  // existing pre-START buffering behaviour. Calling PublishOtaMgrInfo
  // before Initialize() should buffer the message rather than crash on a
  // null DataWriter.
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  EXPECT_NO_THROW(svc.PublishOtaMgrInfo(0, "hello"));
  EXPECT_NO_THROW(svc.PublishOtaMgrInfo(7, "warning sample"));
}

// ===== QA: DDS mock 주입으로 publish 경로 커버 (docs/qa) =====
// MockOtamDdsImpl(OtamDds) 를 test seam 으로 주입해 실 FastDDS 없이 발행 경로 검증.
using ::testing::_;

namespace {
constexpr const char* kInfoTopic =
    "/mmf/otamanager/diagnostic_service/NotifyOtaMgrInfo";
constexpr const char* kHeartbeatTopic =
    "/mmf/otamanager/diagnostic_service/NotifyOtaMgrHeartbeat";
}  // namespace

// PublishHeartbeatOnce: STARTED + mock DDS → Write 1회, seq 증가.
TEST(OtamDiagnosticServiceDds, PublishHeartbeatOnce_MockDds_WritesAndIncrementsSeq) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  auto mock = std::make_shared<MockOtamDdsImpl>();
  EXPECT_CALL(*mock, Write(_)).Times(1);
  svc.InjectDdsForTest(kHeartbeatTopic, mock);
  svc.ForceStartedForTest();
  EXPECT_EQ(svc.heartbeat_seq(), 0);
  svc.PublishHeartbeatOnce();
  EXPECT_EQ(svc.heartbeat_seq(), 1);
}

// PublishHeartbeatOnce: 미기동이면 early return (Write 없음).
TEST(OtamDiagnosticServiceDds, PublishHeartbeatOnce_NotStarted_NoWrite) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  auto mock = std::make_shared<MockOtamDdsImpl>();
  EXPECT_CALL(*mock, Write(_)).Times(0);
  svc.InjectDdsForTest(kHeartbeatTopic, mock);
  svc.PublishHeartbeatOnce();  // state != STARTED
}

// PublishHeartbeatOnce: STARTED 지만 DDS 미등록이면 skip (missing 분기).
TEST(OtamDiagnosticServiceDds, PublishHeartbeatOnce_StartedNoDds_NoThrow) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  svc.ForceStartedForTest();
  EXPECT_NO_THROW(svc.PublishHeartbeatOnce());  // GetDds → null → skip
}

// PublishOtaMgrInfo: STARTED + 구독자 있음 + mock DDS → Write 1회.
TEST(OtamDiagnosticServiceDds, PublishOtaMgrInfo_StartedMatched_MockDds_Writes) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  auto mock = std::make_shared<MockOtamDdsImpl>();
  EXPECT_CALL(*mock, Write(_)).Times(1);
  svc.InjectDdsForTest(kInfoTopic, mock);
  svc.ForceStartedForTest();
  svc.ForceMatchedForTest(1);
  svc.PublishOtaMgrInfo(1, "success sample");
}

// PublishOtaMgrInfo: STARTED 지만 구독자 없음 → 버퍼링 (Write 없음).
TEST(OtamDiagnosticServiceDds, PublishOtaMgrInfo_StartedNoMatch_Buffers) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  auto mock = std::make_shared<MockOtamDdsImpl>();
  EXPECT_CALL(*mock, Write(_)).Times(0);
  svc.InjectDdsForTest(kInfoTopic, mock);
  svc.ForceStartedForTest();  // matched_count_ = 0 (default)
  svc.PublishOtaMgrInfo(2, "buffered sample");
}

// PublishOtaMgrInfo: STARTED + 구독자 있음 지만 DDS 미등록 → "not found" 분기.
TEST(OtamDiagnosticServiceDds, PublishOtaMgrInfo_StartedMatched_NoDds_NoThrow) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  svc.ForceStartedForTest();
  svc.ForceMatchedForTest(1);
  EXPECT_NO_THROW(svc.PublishOtaMgrInfo(3, "no dds"));  // GetDds → null
}

// ── heartbeat 스레드 lifecycle: 미기동 상태로 start→stop (PublishHeartbeatOnce
//    는 no-op → DDS·Write 없음 → 스레드 함수만 커버, race 없음) ──
TEST(OtamDiagnosticServiceDds, HeartbeatThread_StartStop_NoThrow) {
  HeartbeatEnv guard{"1"};  // 1s interval
  OtamDiagnosticService svc;  // NOT started → 루프의 PublishHeartbeatOnce 는 즉시 반환
  EXPECT_NO_THROW(svc.StartHeartbeatForTest());
  EXPECT_NO_THROW(svc.StopHeartbeatForTest());
}

// ── StopHeartbeat: 시작 안 했으면 no-op ──
TEST(OtamDiagnosticServiceDds, StopHeartbeat_NotRunning_NoThrow) {
  HeartbeatEnv guard{nullptr};
  OtamDiagnosticService svc;
  EXPECT_NO_THROW(svc.StopHeartbeatForTest());
}

// ── StartHeartbeat 중복 호출: 두 번째는 이미 실행 중이라 no-op ──
TEST(OtamDiagnosticServiceDds, StartHeartbeat_Twice_SecondIsNoOp) {
  HeartbeatEnv guard{"1"};
  OtamDiagnosticService svc;
  svc.StartHeartbeatForTest();
  EXPECT_NO_THROW(svc.StartHeartbeatForTest());  // already running 분기
  svc.StopHeartbeatForTest();
}
