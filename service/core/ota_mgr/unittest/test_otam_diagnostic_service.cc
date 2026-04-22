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

#include <cstdlib>
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
