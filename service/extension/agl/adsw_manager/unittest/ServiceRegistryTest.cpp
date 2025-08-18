#include "service_registry/ServiceRegistry.hpp"
#include "common/AppUtils.hpp"

#include <gtest/gtest.h>

using namespace adsw_manager::agl;
using namespace adsw_manager_agl;

class ServiceRegistryTest : public ::testing::Test {
protected:
    ServiceRegistry registry;

    void SetUp() override {
        registry.Initialize();
    }
};

// ----------------------------- GetAppList -----------------------------

TEST_F(ServiceRegistryTest, Initialize_PopulatesInitialApps) {
    auto app_list = registry.GetAppList();
    EXPECT_EQ(app_list.size(), 3);
    EXPECT_EQ(app_list[0].app_name(), "adsw_monitor");
    EXPECT_EQ(app_list[1].app_name(), "localization");
    EXPECT_EQ(app_list[2].app_name(), "helloooo");
}

// ----------------------------- AddApp -----------------------------

TEST_F(ServiceRegistryTest, AddApp_AddsNewApp) {
    AppInfo info;
    info.app_name("perception");
    info.app_path("/path/to/perception");
    info.app_state(AppState::DETACHED);

    registry.AddApp(info);
    auto list = registry.GetAppList();

    // 총 4개가 돼야 함
    EXPECT_EQ(list.size(), 4);
    EXPECT_EQ(list.back().app_name(), "perception");
}

TEST_F(ServiceRegistryTest, AddApp_DuplicateName_DoesNotAddAgain) {
    AppInfo info;
    info.app_name("adsw_monitor");  // 이미 존재함
    info.app_path("/some_path");
    info.app_state(AppState::DETACHED);

    registry.AddApp(info);

    auto list = registry.GetAppList();

    // 여전히 3개여야 함 (중복 안 들어감)
    EXPECT_EQ(list.size(), 3);
}

// ----------------------------- RemoveApp -----------------------------

TEST_F(ServiceRegistryTest, RemoveApp_RemovesCorrectApp) {
    registry.RemoveApp("localization");

    auto list = registry.GetAppList();

    EXPECT_EQ(list.size(), 2);
    for (const auto& app : list) {
        EXPECT_NE(app.app_name(), "localization");
    }
}

// ----------------------------- UpdateAppState -----------------------------

TEST_F(ServiceRegistryTest, UpdateAppState_ChangesAppState) {
    registry.UpdateAppState("adsw_monitor", AppState::DETACHED);

    const AppInfo* info = registry.GetAppInfo("adsw_monitor");
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->app_state(), AppState::DETACHED);
}

TEST_F(ServiceRegistryTest, UpdateAppState_UnknownApp_LogsError) {
    // 콘솔 로그 확인은 어렵지만 동작 자체가 죽지 않는지 확인
    registry.UpdateAppState("nonexistent", AppState::DETACHED);
    EXPECT_TRUE(true); // 죽지만 않으면 통과
}

// ----------------------------- GetAppInfo -----------------------------

TEST_F(ServiceRegistryTest, GetAppInfo_ReturnsCorrectPointer) {
    const AppInfo* info = registry.GetAppInfo("helloooo");
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->app_name(), "helloooo");
}

TEST_F(ServiceRegistryTest, GetAppInfo_Nonexistent_ReturnsNull) {
    const AppInfo* info = registry.GetAppInfo("nonexistent");
    EXPECT_EQ(info, nullptr);
}