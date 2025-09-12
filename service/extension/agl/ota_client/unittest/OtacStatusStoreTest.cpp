#include "OtacStatusStore.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

using namespace ota_client::agl;

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
