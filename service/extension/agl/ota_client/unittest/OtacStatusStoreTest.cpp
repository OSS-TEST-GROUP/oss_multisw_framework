#include "OtacStatusStore.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

using namespace ota_client::agl;

namespace {
// QA(docs/qa): config 파일 로딩 경로 커버용 — 임시 JSON 파일을 쓰고 경로를 돌려준다.
std::string WriteTmpConfig(const std::string& name, const std::string& content) {
    std::string path = std::string(::testing::TempDir()) + "/otac_" + name + ".json";
    std::ofstream(path) << content;
    return path;
}
}  // namespace

TEST(OtacStatusStore, InitializeSeedsDefaultApp) {
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize());

    auto names = s.GetAppNameList();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "adsw_monitor");

    auto st = s.GetStatus("adsw_monitor");
    ASSERT_TRUE(st.has_value());
    EXPECT_EQ(*st, "installed");
}

TEST(OtacStatusStore, GetStatusOfMissingKeyReturnsNulloptAndDoesNotInsert) {
    OtacStatusStore s;
    s.Initialize();

    const auto before_size = s.GetAppNameList().size();
    auto st = s.GetStatus("unknown_app");
    EXPECT_FALSE(st.has_value());                 // 값 없음
    const auto after_size = s.GetAppNameList().size();
    EXPECT_EQ(after_size, before_size);           // 조회가 map을 변경하지 않음
}

TEST(OtacStatusStore, UpdateStatusCreatesNewEntry) {
    OtacStatusStore s;
    s.Initialize();

    const auto before_size = s.GetAppNameList().size();
    EXPECT_TRUE(s.UpdateStatus("nav", "pending"));

    auto st = s.GetStatus("nav");
    ASSERT_TRUE(st.has_value());
    EXPECT_EQ(*st, "pending");

    const auto after_size = s.GetAppNameList().size();
    EXPECT_EQ(after_size, before_size + 1);
}

TEST(OtacStatusStore, UpdateStatusUpdatesExistingEntry) {
    OtacStatusStore s;
    s.Initialize();

    EXPECT_TRUE(s.UpdateStatus("adsw_monitor", "updated"));
    auto st = s.GetStatus("adsw_monitor");
    ASSERT_TRUE(st.has_value());
    EXPECT_EQ(*st, "updated");
}

TEST(OtacStatusStore, GetAppNameListReturnsSortedByKey_MapGuarantee) {
    OtacStatusStore s;
    s.Initialize();
    s.UpdateStatus("b_app", "x");
    s.UpdateStatus("a_app", "y");
    s.UpdateStatus("z_app", "z");

    auto names = s.GetAppNameList();
    EXPECT_TRUE(std::is_sorted(names.begin(), names.end()));  // std::map은 키 정렬 보장

    // 필수 키 포함 확인
    for (auto want : {"a_app", "adsw_monitor", "b_app", "z_app"}) {
        EXPECT_NE(std::find(names.begin(), names.end(), std::string{want}), names.end());
    }
}

TEST(OtacStatusStore, ConstQualifiedAccessorsWork) {
    OtacStatusStore s;
    s.Initialize();
    s.UpdateStatus("nav", "ok");

    const OtacStatusStore& cs = s;   // const 객체에서 호출 가능해야 함
    auto st = cs.GetStatus("nav");
    ASSERT_TRUE(st.has_value());
    EXPECT_EQ(*st, "ok");

    auto names = cs.GetAppNameList();
    EXPECT_NE(std::find(names.begin(), names.end(), std::string{"nav"}), names.end());
}

// ── QA(docs/qa): Initialize(config_path) JSON 로딩 경로 커버 ──

TEST(OtacStatusStore, InitializeFromValidJsonConfig) {
    auto path = WriteTmpConfig("valid",
        R"({"client_name":"my_client","apps":[)"
        R"({"name":"nav","status":"installed"},)"
        R"({"name":"media","status":"pending"}]})");
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize(path));
    EXPECT_EQ(s.GetClientName(), "my_client");
    ASSERT_TRUE(s.GetStatus("nav").has_value());
    EXPECT_EQ(*s.GetStatus("nav"), "installed");
    EXPECT_EQ(*s.GetStatus("media"), "pending");
    std::remove(path.c_str());
}

TEST(OtacStatusStore, InitializeNonexistentFileFallsBackToDefaults) {
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize("/nonexistent/dir/otac_missing.json"));
    // file.is_open() == false → LoadDefaults
    EXPECT_TRUE(s.GetStatus("adsw_monitor").has_value());
}

TEST(OtacStatusStore, InitializeEmptyAppsArrayFallsBackToDefaults) {
    auto path = WriteTmpConfig("emptyapps", R"({"client_name":"c","apps":[]})");
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize(path));
    EXPECT_EQ(s.GetClientName(), "c");
    // app_status.empty() → LoadDefaults
    EXPECT_TRUE(s.GetStatus("adsw_monitor").has_value());
    std::remove(path.c_str());
}

TEST(OtacStatusStore, InitializeMalformedJsonFallsBackToDefaults) {
    auto path = WriteTmpConfig("malformed", "{ this is not valid json ");
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize(path));
    // catch(json::exception) → clear + LoadDefaults
    EXPECT_TRUE(s.GetStatus("adsw_monitor").has_value());
    std::remove(path.c_str());
}

TEST(OtacStatusStore, InitializeAppWithoutStatusIsSkipped) {
    // "name" 만 있고 "status" 없는 항목은 && 조건에서 걸러진다(false 측 커버).
    auto path = WriteTmpConfig("nostatus",
        R"({"apps":[{"name":"noStatus"},{"name":"ok","status":"done"}]})");
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize(path));
    EXPECT_FALSE(s.GetStatus("noStatus").has_value());
    ASSERT_TRUE(s.GetStatus("ok").has_value());
    EXPECT_EQ(*s.GetStatus("ok"), "done");
    std::remove(path.c_str());
}

TEST(OtacStatusStore, InitializeClientNameNonStringIgnored) {
    // client_name 이 문자열이 아니면 is_string() false → 반영 안 함(기본 client_name).
    auto path = WriteTmpConfig("cnint",
        R"({"client_name":123,"apps":[{"name":"a","status":"s"}]})");
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize(path));
    EXPECT_EQ(s.GetClientName(), "agl_ota_client");  // 기본값(반영 안 됨)
    EXPECT_TRUE(s.GetStatus("a").has_value());
    std::remove(path.c_str());
}

TEST(OtacStatusStore, GetClientNameReturnsDefaultWhenUninitialized) {
    OtacStatusStore s;  // client_name_ 비어있음
    EXPECT_EQ(s.GetClientName(), "agl_ota_client");
}

TEST(OtacStatusStore, LoadDefaultsPreservesExistingClientName) {
    // Initialize(빈 경로) 는 LoadDefaults 호출 — client_name 이 이미 있으면 유지.
    auto path = WriteTmpConfig("cnonly", R"({"client_name":"keep_me","apps":[]})");
    OtacStatusStore s;
    ASSERT_TRUE(s.Initialize(path));  // client_name 설정 후 apps 비어 LoadDefaults
    EXPECT_EQ(s.GetClientName(), "keep_me");  // LoadDefaults 가 덮어쓰지 않음
    std::remove(path.c_str());
}
