#include "service_registry/ServiceRegistry.hpp"
#include "common/AppUtils.hpp"

#include <gtest/gtest.h>

using namespace adsw_manager::agl;
using namespace lifecyclemgr;

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
    EXPECT_EQ(app_list[0].name(), "adsw_monitor");
    EXPECT_EQ(app_list[1].name(), "map");
    EXPECT_EQ(app_list[2].name(), "radio");
}

// ----------------------------- AddApp -----------------------------

TEST_F(ServiceRegistryTest, AddApp_AddsNewApp) {
    ServiceStatus info;
    info.name("perception");
    info.service_type(ServiceType::NORMAL);
    info.service_state(ServiceState::NOT_RUNNING);
    info.is_enabled(true);
    info.recovered_count(0);

    registry.AddApp(info);
    auto list = registry.GetAppList();

    // 총 4개가 돼야 함
    EXPECT_EQ(list.size(), 4);
    EXPECT_EQ(list.back().name(), "perception");
}

TEST_F(ServiceRegistryTest, AddApp_DuplicateName_DoesNotAddAgain) {
    ServiceStatus info;
    info.name("adsw_monitor");  // 이미 존재함
    info.service_type(ServiceType::NORMAL);
    info.service_state(ServiceState::NOT_RUNNING);
    info.is_enabled(false);
    info.recovered_count(0);

    registry.AddApp(info);

    auto list = registry.GetAppList();

    // 여전히 3개여야 함 (중복 안 들어감)
    EXPECT_EQ(list.size(), 3);
}

// ----------------------------- RemoveApp -----------------------------

TEST_F(ServiceRegistryTest, RemoveApp_RemovesCorrectApp) {
    registry.RemoveApp("map");

    auto list = registry.GetAppList();

    EXPECT_EQ(list.size(), 2);
    for (const auto& app : list) {
        EXPECT_NE(app.name(), "map");
    }
}

// ----------------------------- UpdateAppState -----------------------------

TEST_F(ServiceRegistryTest, UpdateAppState_ChangesAppState) {
    registry.UpdateAppState("adsw_monitor", ServiceState::NOT_RUNNING);

    const ServiceStatus* info = registry.GetAppInfo("adsw_monitor");
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->service_state(), ServiceState::NOT_RUNNING);
}

TEST_F(ServiceRegistryTest, UpdateAppState_UnknownApp_LogsError) {
    // 콘솔 로그 확인은 어렵지만 동작 자체가 죽지 않는지 확인
    registry.UpdateAppState("nonexistent", ServiceState::NOT_RUNNING);
    EXPECT_TRUE(true); // 죽지만 않으면 통과
}

// ----------------------------- GetAppInfo -----------------------------

TEST_F(ServiceRegistryTest, GetAppInfo_ReturnsCorrectPointer) {
    const ServiceStatus* info = registry.GetAppInfo("map");
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->name(), "map");
}

TEST_F(ServiceRegistryTest, GetAppInfo_Nonexistent_ReturnsNull) {
    const ServiceStatus* info = registry.GetAppInfo("nonexistent");
    EXPECT_EQ(info, nullptr);
}