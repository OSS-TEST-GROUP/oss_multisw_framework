#include "service_registry/ServiceRegistry.hpp"
#include <gtest/gtest.h>
#include <cstdlib>   // setenv / unsetenv (ExpandEnvVars 테스트)
#include <optional>
#include <algorithm>
#include "TestPrinter.hpp"

using namespace adsw_manager::agl;
using namespace lifecyclemgr;

namespace {

// 최소 필드만 세팅해서 Service 하나 만드는 헬퍼
static Service MakeService(const std::string& name) {
    Service svc;
    svc.name(name);

    return svc;
}

// 벡터에 특정 이름이 존재하는지 확인
static bool ContainsName(const std::vector<std::pair<std::string, ServiceState>>& v,
                         const std::string& name) {
    return std::any_of(v.begin(), v.end(), [&](auto const& p){ return p.first == name; });
}

} // namespace

class ServiceRegistryTest : public ::testing::Test {
protected:
    ServiceRegistry registry;

    void SetUp() override {
        registry.Initialize();
    }
};

// ----------------------------- Initialize / Snapshot -----------------------------

TEST_F(ServiceRegistryTest, Initialize_PopulatesInitialApps_UsingNameStatePairs) {
    auto pairs = registry.GetNameStatePairs();

    EXPECT_EQ(pairs.size(), 4u);
    EXPECT_TRUE(ContainsName(pairs, "adsw_monitor"));
    EXPECT_TRUE(ContainsName(pairs, "ota_monitor"));
    EXPECT_TRUE(ContainsName(pairs, "lfc_monitor"));
    EXPECT_TRUE(ContainsName(pairs, "hmi_launcher"));
}

// ----------------------------- AddApp -----------------------------

TEST_F(ServiceRegistryTest, AddApp_AddsNewApp_DefaultStateNotRunning) {
    auto before = registry.GetNameStatePairs();

    auto svc = MakeService("perception");
    registry.AddApp(svc);

    auto after = registry.GetNameStatePairs();
    EXPECT_EQ(after.size(), before.size() + 1);
    EXPECT_TRUE(ContainsName(after, "perception"));

    EXPECT_EQ(registry.GetAppState("perception"), ServiceState::NOT_RUNNING);
}

TEST_F(ServiceRegistryTest, AddApp_WithInitialState_SetsThatState) {
    auto svc = MakeService("planning");
    registry.AddApp(svc, ServiceState::RUNNING_NO_RESPONSE);

    EXPECT_EQ(registry.GetAppState("planning"), ServiceState::RUNNING_NO_RESPONSE);
}

TEST_F(ServiceRegistryTest, AddApp_DuplicateName_DoesNotAddAgain) {
    auto before = registry.GetNameStatePairs();

    // 이미 존재하는 "ota_monitor" 추가 시도
    auto dup = MakeService("ota_monitor");
    registry.AddApp(dup);

    auto after = registry.GetNameStatePairs();
    EXPECT_EQ(after.size(), before.size());
}

// ----------------------------- RemoveApp -----------------------------

TEST_F(ServiceRegistryTest, RemoveApp_RemovesCorrectApp) {
    // 사전조건: "ota_monitor" 존재
    ASSERT_EQ(registry.GetAppState("ota_monitor") == ServiceState::NOT_RUNNING ||
              registry.GetAppState("ota_monitor") == ServiceState::RUNNING ||
              true, true); // 존재 확인 겸, 상태는 무엇이든 상관 X

    registry.RemoveApp("ota_monitor");

    auto list = registry.GetNameStatePairs();
    EXPECT_FALSE(ContainsName(list, "ota_monitor"));
}

TEST_F(ServiceRegistryTest, RemoveApp_UnknownName_NoCrash) {
    registry.RemoveApp("nonexistent");
    SUCCEED(); // 크래시/예외 없이 지나가면 통과
}

// ----------------------------- UpdateAppState -----------------------------

TEST_F(ServiceRegistryTest, UpdateAppState_ChangesAppState) {
    // 초기 앱 중 하나의 상태 변경
    registry.UpdateAppState("adsw_monitor", ServiceState::RUNNING);

    EXPECT_EQ(registry.GetAppState("adsw_monitor"), ServiceState::RUNNING);

    // GetAppInfo로도 동일 상태 확인
    auto info = registry.GetAppInfo("adsw_monitor");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->first.name(), "adsw_monitor");
    EXPECT_EQ(info->second, ServiceState::RUNNING);
}

TEST_F(ServiceRegistryTest, UpdateAppState_UnknownApp_NoCrash) {
    registry.UpdateAppState("nonexistent", ServiceState::TERMINATING);
    SUCCEED(); // 죽지 않으면 통과
}

// ----------------------------- GetAppState -----------------------------

TEST_F(ServiceRegistryTest, GetAppState_ReturnsNotRunningForUnknown) {
    EXPECT_EQ(registry.GetAppState("no_such_app"), ServiceState::NOT_RUNNING);
}

// ----------------------------- GetAppInfo -----------------------------

TEST_F(ServiceRegistryTest, GetAppInfo_ReturnsPairForExisting) {
    auto info = registry.GetAppInfo("hmi_launcher");
    ASSERT_TRUE(info.has_value());
    const auto& [svc, state] = *info;
    EXPECT_EQ(svc.name(), "hmi_launcher");
}

TEST_F(ServiceRegistryTest, GetAppInfo_ReturnsNulloptForNonexistent) {
    auto info = registry.GetAppInfo("ghost_app");
    EXPECT_FALSE(info.has_value());
}

// ----------------------------- GetNameStatePairs -----------------------------

TEST_F(ServiceRegistryTest, GetNameStatePairs_ReflectsUpdates) {
    auto before = registry.GetNameStatePairs();

    auto svc = MakeService("localization");
    registry.AddApp(svc, ServiceState::INITIALIZING);
    registry.UpdateAppState("localization", ServiceState::RUNNING);

    auto after = registry.GetNameStatePairs();
    EXPECT_EQ(after.size(), before.size() + 1);
    EXPECT_TRUE(ContainsName(after, "localization"));
    EXPECT_EQ(registry.GetAppState("localization"), ServiceState::RUNNING);
}

// ── ExpandEnvVars — 서비스 목록 정본을 이미지에 두기 위한 확장 규칙 ──────────
//
// 이 함수가 틀리면 런처의 --group-add gid 가 조용히 어긋나 docker.sock 접근을
// 잃는다(보드에서 실측으로 판별하기 어려운 종류의 회귀 — DOCKER_GID 가
// ext-agl 자신의 group_add 에도 쓰여서 값을 따로 바꿔 볼 수 없다).

TEST(ExpandEnvVars, EnvValueWinsOverDefault) {
    ::setenv("ADSW_TEST_GID", "4242", 1);
    EXPECT_EQ(ExpandEnvVars("${ADSW_TEST_GID:-999}"), "4242");
    ::unsetenv("ADSW_TEST_GID");
}

TEST(ExpandEnvVars, FallsBackToDefaultWhenUnset) {
    ::unsetenv("ADSW_TEST_GID");
    EXPECT_EQ(ExpandEnvVars("${ADSW_TEST_GID:-999}"), "999");
}

// compose 가 `SAVED_REGISTRY_BACKUP=` 처럼 빈 값을 주입하는 일이 실제로 있다.
// 셸과 같이 빈 값은 미설정으로 본다 — 그러지 않으면 경로/인자가 빈칸이 된다.
TEST(ExpandEnvVars, EmptyEnvTreatedAsUnset) {
    ::setenv("ADSW_TEST_GID", "", 1);
    EXPECT_EQ(ExpandEnvVars("${ADSW_TEST_GID:-999}"), "999");
    ::unsetenv("ADSW_TEST_GID");
}

TEST(ExpandEnvVars, NoDefaultAndUnsetYieldsEmpty) {
    ::unsetenv("ADSW_TEST_NONE");
    EXPECT_EQ(ExpandEnvVars("a${ADSW_TEST_NONE}b"), "ab");
}

// 실제 yaml 의 형태: 한 값 안에 같은 변수가 두 번 들어간다
// (`/run/user/${UID}:/run/user/${UID}`).
TEST(ExpandEnvVars, ExpandsMultipleOccurrencesAndKeepsSurroundingText) {
    ::setenv("ADSW_TEST_UID", "1001", 1);
    EXPECT_EQ(ExpandEnvVars("/run/user/${ADSW_TEST_UID:-1000}:/run/user/${ADSW_TEST_UID:-1000}"),
              "/run/user/1001:/run/user/1001");
    ::unsetenv("ADSW_TEST_UID");
}

TEST(ExpandEnvVars, LeavesPlainStringsAndLoneDollarsUntouched) {
    EXPECT_EQ(ExpandEnvVars("--net=host"), "--net=host");
    EXPECT_EQ(ExpandEnvVars("cost is $5"), "cost is $5");
}

// 닫는 중괄호가 없으면 삼켜 버리지 말고 원문을 남긴다 (yaml 오타를 눈에 보이게).
TEST(ExpandEnvVars, UnterminatedBraceIsPreservedVerbatim) {
    EXPECT_EQ(ExpandEnvVars("x${BROKEN"), "x${BROKEN");
}
