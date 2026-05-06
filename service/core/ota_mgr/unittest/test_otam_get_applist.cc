// F022 — unit tests for OtamClientService::BuildGetAppListResponse.
//
// Pure aggregator: no DDS, no participant, no fastrtps wiring. We feed
// it a synthetic snapshot of (client_id → OtaClientInfo) and assert on
// the parallel arrays + ret_code it produces.
//
// 4 cases (one per F022 ac4 bullet):
//   - empty input (no clients registered)
//   - single client with N apps (sort + parallel-array invariant)
//   - multiple clients with overlapping apps (dedupe + "lowest wins")
//   - 1000 clients × 5 apps each (performance smoke; completes <1s)

#include "otam_client_service/otam_client_service.h"

#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

OtaClientInfo make_client(std::string name,
                          std::vector<std::string> apps) {
  return OtaClientInfo{std::move(name), std::move(apps)};
}

}  // namespace

TEST(OtamClientServiceBuildGetAppList, EmptySnapshotReturnsSuccessNoApps) {
  std::map<int32_t, OtaClientInfo> empty;
  auto resp = OtamClientService::BuildGetAppListResponse(42, empty);

  EXPECT_EQ(resp.id(), 42);
  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);
  EXPECT_TRUE(resp.app_names().empty());
  EXPECT_TRUE(resp.owner_client_ids().empty());
  EXPECT_EQ(resp.app_names().size(), resp.owner_client_ids().size());
}

TEST(OtamClientServiceBuildGetAppList,
     SingleClientYieldsSortedNamesWithSameOwner) {
  std::map<int32_t, OtaClientInfo> clients;
  clients[7] = make_client("client_a",
                           {"navi", "audio", "music", "video", "weather"});

  auto resp = OtamClientService::BuildGetAppListResponse(100, clients);

  EXPECT_EQ(resp.id(), 100);
  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);

  // 5 distinct apps, sorted lexicographically.
  const std::vector<std::string> expected = {
      "audio", "music", "navi", "video", "weather"};
  ASSERT_EQ(resp.app_names().size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(resp.app_names()[i], expected[i]) << "idx=" << i;
  }
  // Parallel-array invariant: every owner is client 7.
  ASSERT_EQ(resp.owner_client_ids().size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(resp.owner_client_ids()[i], 7) << "idx=" << i;
  }
}

TEST(OtamClientServiceBuildGetAppList,
     OverlappingClientsDedupeAndLowestIdWins) {
  std::map<int32_t, OtaClientInfo> clients;
  // Insert in non-monotonic order to make sure the algorithm doesn't
  // accidentally pick whichever was iterated first.
  clients[10] = make_client("ten", {"shared_app", "ten_only"});
  clients[3]  = make_client("three", {"shared_app", "three_only"});
  clients[7]  = make_client("seven", {"shared_app", "seven_only"});

  auto resp = OtamClientService::BuildGetAppListResponse(7, clients);

  EXPECT_EQ(resp.id(), 7);
  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);

  // 4 distinct apps, alphabetically sorted.
  const std::vector<std::string> expected_names = {
      "seven_only", "shared_app", "ten_only", "three_only"};
  ASSERT_EQ(resp.app_names().size(), expected_names.size());
  for (size_t i = 0; i < expected_names.size(); ++i) {
    EXPECT_EQ(resp.app_names()[i], expected_names[i]) << "idx=" << i;
  }

  // For shared_app the owner MUST be client 3 (the lowest id).
  ASSERT_EQ(resp.owner_client_ids().size(), expected_names.size());
  for (size_t i = 0; i < expected_names.size(); ++i) {
    if (resp.app_names()[i] == "shared_app") {
      EXPECT_EQ(resp.owner_client_ids()[i], 3)
          << "shared_app must be owned by lowest client_id (3)";
    } else if (resp.app_names()[i] == "ten_only") {
      EXPECT_EQ(resp.owner_client_ids()[i], 10);
    } else if (resp.app_names()[i] == "three_only") {
      EXPECT_EQ(resp.owner_client_ids()[i], 3);
    } else if (resp.app_names()[i] == "seven_only") {
      EXPECT_EQ(resp.owner_client_ids()[i], 7);
    }
  }
}

TEST(OtamClientServiceBuildGetAppList,
     ThousandClientsCompleteUnderOneSecond) {
  std::map<int32_t, OtaClientInfo> clients;
  // 1000 clients × 5 apps each. Many apps are shared across clients to
  // exercise the dedupe path; some are unique so the result still grows.
  for (int32_t c = 0; c < 1000; ++c) {
    std::vector<std::string> apps;
    apps.reserve(5);
    apps.push_back("global_app");           // every client shares this one
    apps.push_back("group_" + std::to_string(c % 10));   // 10 groups
    apps.push_back("dual_" + std::to_string(c / 500));   // 2 buckets
    apps.push_back("uniq_a_" + std::to_string(c));       // unique
    apps.push_back("uniq_b_" + std::to_string(c));       // unique
    clients[c] = make_client("client_" + std::to_string(c), std::move(apps));
  }

  const auto t0 = std::chrono::steady_clock::now();
  auto resp = OtamClientService::BuildGetAppListResponse(999, clients);
  const auto t1 = std::chrono::steady_clock::now();
  const auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);
  EXPECT_EQ(resp.id(), 999);

  // Expected unique app count:
  //   1 (global_app)
  // + 10 (group_0..9)
  // + 2  (dual_0..1)
  // + 1000 uniq_a + 1000 uniq_b
  // = 2013
  EXPECT_EQ(resp.app_names().size(), 2013u);
  EXPECT_EQ(resp.owner_client_ids().size(), resp.app_names().size());

  // Performance smoke: 1k×5 inputs must aggregate well under a second
  // even on modest hardware.
  EXPECT_LT(ms, 1000)
      << "BuildGetAppListResponse took " << ms << "ms for 1000 clients";

  // Spot-check: shared apps should be owned by the lowest client_id
  // that registered them. global_app is registered by every client,
  // so the lowest is 0.
  std::map<std::string, int32_t> got;
  for (size_t i = 0; i < resp.app_names().size(); ++i) {
    got[resp.app_names()[i]] = resp.owner_client_ids()[i];
  }
  EXPECT_EQ(got["global_app"], 0);
  // group_0 is registered by clients with c % 10 == 0, lowest is 0.
  EXPECT_EQ(got["group_0"], 0);
  // group_5 is registered by 5, 15, 25, ... lowest is 5.
  EXPECT_EQ(got["group_5"], 5);
  // dual_1 is registered by clients 500..999, lowest is 500.
  EXPECT_EQ(got["dual_1"], 500);
}
