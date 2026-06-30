// Unit tests for OtamActorService::BuildGetAppListResponse.
//
// Pure aggregator: no DDS, no participant, no fastrtps wiring. We feed
// it a synthetic snapshot of (client_id → OtaClientInfo) and assert on
// the parallel arrays + ret_code it produces.
//
// Contract (multi-host): an app name is unique only within a single host,
// where exactly one client registers it and apps are never shared between
// clients. Across heterogeneous hosts (e.g. a ros2 board and an agl board)
// the same app name can legitimately exist, each registered by its own
// client. The aggregator therefore emits ONE entry per (client, app) pair —
// it does NOT dedupe by app name — so every host's installation stays
// visible. Entries are ordered by ascending client_id (std::map iteration)
// then registration order within a client.
//
// 4 cases:
//   - empty input (no clients registered)
//   - single client with N apps (order + parallel-array invariant)
//   - heterogeneous clients sharing an app name (all entries appear)
//   - 1000 clients × 5 apps each (no-dedupe count + performance smoke)
//
// Note: BuildGetAppListResponse was relocated from OtamClientService
// to OtamActorService when GetAppList moved to the otaactor_service
// topic namespace.

#include "otam_actor_service/otam_actor_service.h"

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
  auto resp = OtamActorService::BuildGetAppListResponse(42, empty);

  EXPECT_EQ(resp.id(), 42);
  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);
  EXPECT_TRUE(resp.app_names().empty());
  EXPECT_TRUE(resp.owner_client_ids().empty());
  EXPECT_EQ(resp.app_names().size(), resp.owner_client_ids().size());
}

TEST(OtamClientServiceBuildGetAppList,
     SingleClientYieldsAppsInRegistrationOrderSameOwner) {
  std::map<int32_t, OtaClientInfo> clients;
  clients[7] = make_client("client_a",
                           {"navi", "audio", "music", "video", "weather"});

  auto resp = OtamActorService::BuildGetAppListResponse(100, clients);

  EXPECT_EQ(resp.id(), 100);
  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);

  // 5 apps, emitted in registration order (no sort, no dedupe).
  const std::vector<std::string> expected = {
      "navi", "audio", "music", "video", "weather"};
  ASSERT_EQ(resp.app_names().size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(resp.app_names()[i], expected[i]) << "idx=" << i;
  }
  // Parallel-array invariant: every owner is client 7 / "client_a".
  ASSERT_EQ(resp.owner_client_ids().size(), expected.size());
  ASSERT_EQ(resp.owner_client_names().size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(resp.owner_client_ids()[i], 7) << "idx=" << i;
    EXPECT_EQ(resp.owner_client_names()[i], "client_a") << "idx=" << i;
  }
}

TEST(OtamClientServiceBuildGetAppList,
     HeterogeneousClientsSameAppNameAllAppear) {
  std::map<int32_t, OtaClientInfo> clients;
  // Three clients on three hosts. Each registers the same "shared_app"
  // (e.g. adsw_monitor installed on every board) plus one host-unique app.
  // Insert in non-monotonic order to prove ordering comes from client_id,
  // not insertion order.
  clients[10] = make_client("ten", {"shared_app", "ten_only"});
  clients[3]  = make_client("three", {"shared_app", "three_only"});
  clients[7]  = make_client("seven", {"shared_app", "seven_only"});

  auto resp = OtamActorService::BuildGetAppListResponse(7, clients);

  EXPECT_EQ(resp.id(), 7);
  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);

  // 6 entries total — NO dedupe. Ordered by ascending client_id (3, 7, 10)
  // then registration order within each client.
  const std::vector<std::string> expected_names = {
      "shared_app", "three_only",   // client 3
      "shared_app", "seven_only",   // client 7
      "shared_app", "ten_only"};    // client 10
  const std::vector<int32_t> expected_owners = {3, 3, 7, 7, 10, 10};
  const std::vector<std::string> expected_owner_names = {
      "three", "three", "seven", "seven", "ten", "ten"};

  ASSERT_EQ(resp.app_names().size(), expected_names.size());
  ASSERT_EQ(resp.owner_client_ids().size(), expected_names.size());
  ASSERT_EQ(resp.owner_client_names().size(), expected_names.size());
  for (size_t i = 0; i < expected_names.size(); ++i) {
    EXPECT_EQ(resp.app_names()[i], expected_names[i]) << "idx=" << i;
    EXPECT_EQ(resp.owner_client_ids()[i], expected_owners[i]) << "idx=" << i;
    EXPECT_EQ(resp.owner_client_names()[i], expected_owner_names[i])
        << "idx=" << i;
  }

  // "shared_app" must appear once per host (3 times), each owned by its own
  // client — none hidden.
  std::set<int32_t> shared_owners;
  for (size_t i = 0; i < resp.app_names().size(); ++i) {
    if (resp.app_names()[i] == "shared_app") {
      shared_owners.insert(resp.owner_client_ids()[i]);
    }
  }
  EXPECT_EQ(shared_owners, (std::set<int32_t>{3, 7, 10}));
}

TEST(OtamClientServiceBuildGetAppList,
     ThousandClientsNoDedupeCompleteUnderOneSecond) {
  std::map<int32_t, OtaClientInfo> clients;
  // 1000 clients × 5 apps each. Many app names repeat across clients to
  // exercise the multi-host path; some are unique. With per-(client, app)
  // enumeration the total is exactly 1000 × 5 = 5000 entries.
  for (int32_t c = 0; c < 1000; ++c) {
    std::vector<std::string> apps;
    apps.reserve(5);
    apps.push_back("global_app");           // every client registers this one
    apps.push_back("group_" + std::to_string(c % 10));   // 10 groups
    apps.push_back("dual_" + std::to_string(c / 500));   // 2 buckets
    apps.push_back("uniq_a_" + std::to_string(c));       // unique
    apps.push_back("uniq_b_" + std::to_string(c));       // unique
    clients[c] = make_client("client_" + std::to_string(c), std::move(apps));
  }

  const auto t0 = std::chrono::steady_clock::now();
  auto resp = OtamActorService::BuildGetAppListResponse(999, clients);
  const auto t1 = std::chrono::steady_clock::now();
  const auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

  EXPECT_EQ(resp.ret_code(), ReturnValue::SUCCESS);
  EXPECT_EQ(resp.id(), 999);

  // No dedupe: every (client, app) pair is emitted → 1000 × 5 = 5000.
  EXPECT_EQ(resp.app_names().size(), 5000u);
  EXPECT_EQ(resp.owner_client_ids().size(), resp.app_names().size());
  EXPECT_EQ(resp.owner_client_names().size(), resp.app_names().size());

  // Performance smoke: 1k×5 inputs must aggregate well under a second
  // even on modest hardware.
  EXPECT_LT(ms, 1000)
      << "BuildGetAppListResponse took " << ms << "ms for 1000 clients";

  // "global_app" is registered by all 1000 clients, so it must appear 1000
  // times, each entry owned by a distinct client_id 0..999.
  std::set<int32_t> global_owners;
  size_t global_count = 0;
  for (size_t i = 0; i < resp.app_names().size(); ++i) {
    if (resp.app_names()[i] == "global_app") {
      ++global_count;
      global_owners.insert(resp.owner_client_ids()[i]);
    }
  }
  EXPECT_EQ(global_count, 1000u);
  EXPECT_EQ(global_owners.size(), 1000u);

  // Entries are grouped by ascending client_id: the first 5 belong to
  // client 0, and "global_app" leads each client's block.
  EXPECT_EQ(resp.app_names()[0], "global_app");
  EXPECT_EQ(resp.owner_client_ids()[0], 0);
}
