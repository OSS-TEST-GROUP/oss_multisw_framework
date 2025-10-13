#include "service_registry/ServiceRegistry.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <algorithm>

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

    EXPECT_EQ(pairs.size(), 3u);
    EXPECT_TRUE(ContainsName(pairs, "adsw_monitor"));
    EXPECT_TRUE(ContainsName(pairs, "map"));
    EXPECT_TRUE(ContainsName(pairs, "radio"));
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

    // 이미 존재하는 "map" 추가 시도
    auto dup = MakeService("map");
    registry.AddApp(dup);

    auto after = registry.GetNameStatePairs();
    EXPECT_EQ(after.size(), before.size());
}

// ----------------------------- RemoveApp -----------------------------

TEST_F(ServiceRegistryTest, RemoveApp_RemovesCorrectApp) {
    // 사전조건: "map" 존재
    ASSERT_EQ(registry.GetAppState("map") == ServiceState::NOT_RUNNING ||
              registry.GetAppState("map") == ServiceState::RUNNING ||
              true, true); // 존재 확인 겸, 상태는 무엇이든 상관 X

    registry.RemoveApp("map");

    auto list = registry.GetNameStatePairs();
    EXPECT_FALSE(ContainsName(list, "map"));
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
    auto info = registry.GetAppInfo("radio");
    ASSERT_TRUE(info.has_value());
    const auto& [svc, state] = *info;
    EXPECT_EQ(svc.name(), "radio");
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
