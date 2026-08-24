// Unit tests for the SS_v2 -> canonical EgoState mapping (B2-1).
//
// Deliberately dependency-free (no gtest, no ROS, no DDS) so it runs anywhere:
//     c++ -std=c++20 -I../include test_mapping.cpp -o t && ./t
// or via adapters/ss2/run_tests.sh.  The whole point of ss2_ego_mapping.hpp being pure
// is that this can run on a developer laptop with no simulator, so the barrier to
// running it must be zero.
//
// What is worth testing here is not "does it copy x to x" but the three places this
// mapping can be silently wrong:
//   1. the quaternion convention (a transposed sign is invisible in review)
//   2. ego selection when a scenario has several entities
//   3. the seconds->nanoseconds conversion

#include "ss2_ego_mapping.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("FAIL  %s\n", what.c_str());
    }
}

void check_near(double got, double want, double tol, const std::string& what) {
    ++g_checks;
    if (!(std::abs(got - want) <= tol)) {
        ++g_failures;
        std::printf("FAIL  %s: got %.12g want %.12g (tol %g)\n", what.c_str(), got, want, tol);
    }
}

using namespace sils::ss2;

// Build a quaternion from intrinsic Z-Y-X euler angles -- the inverse of the function
// under test, written independently so a shared sign error can't cancel out.
Quaternion quat_from_rpy(double roll, double pitch, double yaw) {
    const double cr = std::cos(roll * 0.5), sr = std::sin(roll * 0.5);
    const double cp = std::cos(pitch * 0.5), sp = std::sin(pitch * 0.5);
    const double cy = std::cos(yaw * 0.5), sy = std::sin(yaw * 0.5);
    Quaternion q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

// --- 1. orientation ----------------------------------------------------------

void test_quaternion_identity() {
    const Vec3 rpy = quaternion_to_rpy(Quaternion{});
    check_near(rpy.x, 0.0, 1e-12, "identity roll");
    check_near(rpy.y, 0.0, 1e-12, "identity pitch");
    check_near(rpy.z, 0.0, 1e-12, "identity yaw");
}

void test_quaternion_roundtrip() {
    // Ordinary driving attitudes plus a few awkward ones.
    const double cases[][3] = {
        {0.0, 0.0, 0.0},
        {0.0, 0.0, M_PI / 2},          // heading +90 deg -- the common case
        {0.0, 0.0, -M_PI / 2},
        {0.0, 0.0, 3.0},               // near +-pi wrap
        {0.1, -0.2, 0.3},
        {-0.4, 0.15, -2.5},
        {0.05, 0.0, M_PI - 0.01},
    };
    for (const auto& c : cases) {
        const Quaternion q = quat_from_rpy(c[0], c[1], c[2]);
        const Vec3 rpy = quaternion_to_rpy(q);
        const std::string tag = "roundtrip(" + std::to_string(c[0]) + "," +
                                std::to_string(c[1]) + "," + std::to_string(c[2]) + ")";
        check_near(rpy.x, c[0], 1e-9, tag + " roll");
        check_near(rpy.y, c[1], 1e-9, tag + " pitch");
        check_near(rpy.z, c[2], 1e-9, tag + " yaw");
    }
}

void test_quaternion_yaw_sign() {
    // Guards the sign convention explicitly: a +90 deg yaw must come back POSITIVE.
    // A transposed atan2 argument pair still round-trips against its own inverse, so
    // the round-trip test above cannot catch a globally flipped convention -- this can.
    const Quaternion q = quat_from_rpy(0.0, 0.0, M_PI / 2);
    const Vec3 rpy = quaternion_to_rpy(q);
    check(rpy.z > 0.0, "yaw +90deg stays positive (convention, not just self-consistent)");
    check_near(rpy.z, M_PI / 2, 1e-9, "yaw +90deg magnitude");
}

void test_quaternion_unnormalized() {
    // Serialization round-trips leave quaternions slightly off unit length; scaling one
    // by 2 must not scale the angles.
    Quaternion q = quat_from_rpy(0.1, -0.2, 0.3);
    q.x *= 2; q.y *= 2; q.z *= 2; q.w *= 2;
    const Vec3 rpy = quaternion_to_rpy(q);
    check_near(rpy.x, 0.1, 1e-9, "unnormalized roll");
    check_near(rpy.y, -0.2, 1e-9, "unnormalized pitch");
    check_near(rpy.z, 0.3, 1e-9, "unnormalized yaw");
}

void test_quaternion_degenerate_is_not_nan() {
    // All-zero quaternion (uninitialized message) must not produce NaN: NaN in this
    // pipeline means "injected fault" (see EgoStateProvenance).
    const Vec3 rpy = quaternion_to_rpy(Quaternion{0, 0, 0, 0});
    check(std::isfinite(rpy.x) && std::isfinite(rpy.y) && std::isfinite(rpy.z),
          "all-zero quaternion yields finite rpy (never NaN)");
}

void test_quaternion_gimbal_lock_is_not_nan() {
    const Quaternion q = quat_from_rpy(0.0, M_PI / 2, 0.0);
    const Vec3 rpy = quaternion_to_rpy(q);
    check(std::isfinite(rpy.x) && std::isfinite(rpy.y) && std::isfinite(rpy.z),
          "gimbal lock yields finite rpy (never NaN)");
    check_near(rpy.y, M_PI / 2, 1e-6, "gimbal lock pitch");
}

// --- 2. ego selection --------------------------------------------------------

std::vector<Ss2Entity> sample_entities() {
    Ss2Entity npc1;  npc1.name = "npc_a";  npc1.type = EntityType::kVehicle;
    Ss2Entity ego;   ego.name  = "ego";    ego.type  = EntityType::kEgo;
    Ss2Entity npc2;  npc2.name = "npc_b";  npc2.type = EntityType::kPedestrian;
    return {npc1, ego, npc2};
}

void test_select_ego_by_type() {
    const auto entities = sample_entities();
    const Ss2Entity* e = select_ego(entities, "");
    check(e != nullptr && e->name == "ego", "selects the EGO-typed entity, not the first entry");
}

void test_select_ego_by_name() {
    const auto entities = sample_entities();
    const Ss2Entity* e = select_ego(entities, "npc_a");
    check(e != nullptr && e->name == "npc_a", "explicit name wins over type");
}

void test_select_ego_named_but_absent_does_not_fall_back() {
    // If someone asks for a specific ego and it is not there, silently handing back a
    // different entity would publish another car's state under the ego's identity.
    const auto entities = sample_entities();
    check(select_ego(entities, "no_such_entity") == nullptr,
          "named ego absent -> nullptr, no silent fallback to the EGO-typed one");
}

void test_select_ego_empty() {
    check(select_ego({}, "") == nullptr, "empty array -> nullptr");
    Ss2Entity npc; npc.name = "npc"; npc.type = EntityType::kVehicle;
    check(select_ego({npc}, "") == nullptr, "no ego present -> nullptr (not the NPC)");
}

// --- 3. time -----------------------------------------------------------------

void test_time_conversion() {
    check(sim_seconds_to_ns(0.0) == 0, "t=0");
    check(sim_seconds_to_ns(1.0) == 1000000000LL, "t=1s");
    check(sim_seconds_to_ns(0.05) == 50000000LL, "t=50ms (30Hz-ish step)");
    // Rounding, not truncation: 1e-9*3 in binary floating point lands just under.
    check(sim_seconds_to_ns(3e-9) == 3LL, "sub-tick value rounds rather than truncates");
    check(sim_seconds_to_ns(-1.5) == -1500000000LL, "negative passes through unclamped");
}

// --- 4. the whole mapping ----------------------------------------------------

Ss2Entity moving_ego() {
    Ss2Entity e;
    e.name = "ego";
    e.type = EntityType::kEgo;
    e.time_sec = 12.5;
    e.position = Vec3{100.5, -20.25, 0.75};
    e.orientation = quat_from_rpy(0.0, 0.0, M_PI / 4);
    e.twist_linear = Vec3{16.7, 0.2, 0.0};      // m/s
    e.twist_angular = Vec3{0.0, 0.0, 0.35};     // rad/s
    e.accel_linear = Vec3{-3.0, 0.1, 0.0};      // m/s^2
    return e;
}

void test_map_passthrough_fields() {
    const Ss2Entity ego = moving_ego();
    const auto s = map_ego_state(ego, "run-42", 7, SynthesizedDefaults{});

    check(s.run_id == "run-42", "run_id injected");
    check(s.seq == 7, "seq injected");
    check(s.stamp_ns == 12500000000LL, "stamp_ns from EntityStatus.time");

    check_near(s.position.x, 100.5, 1e-12, "position.x 1:1 (m)");
    check_near(s.position.y, -20.25, 1e-12, "position.y 1:1");
    check_near(s.position.z, 0.75, 1e-12, "position.z 1:1");

    // No unit conversion on this source -- assert it explicitly so that a future
    // "helpful" scaling factor breaks a test instead of the verdict.
    check_near(s.velocity.x, 16.7, 1e-12, "velocity 1:1, NOT scaled (m/s in, m/s out)");
    check_near(s.acceleration.x, -3.0, 1e-12, "acceleration 1:1, NOT scaled");
    check_near(s.yaw_rate, 0.35, 1e-12, "yaw_rate from twist.angular.z, rad/s 1:1");

    check_near(s.orientation_rpy.z, M_PI / 4, 1e-9, "yaw from quaternion");
}

void test_map_declares_synthesized_fields() {
    const Ss2Entity ego = moving_ego();
    EgoStateProvenance prov;
    const auto s = map_ego_state(ego, "run-42", 0, SynthesizedDefaults{}, &prov);

    check(prov.steering_angle_synthesized, "steering_angle declared synthesized");
    check(prov.gear_synthesized, "gear declared synthesized");
    check(prov.any(), "provenance.any() true when something was synthesized");
    check(prov.summary() == "steering_angle, gear", "provenance summary text");

    // The synthesized values must be finite and in-contract -- NOT NaN, which this
    // pipeline reads as an injected fault (vehicle_input_fault.hpp).
    check(std::isfinite(s.steering_angle), "synthesized steering_angle is finite, never NaN");
    check(s.gear <= 3, "synthesized gear within 0..3");
}

void test_provenance_summary_when_nothing_synthesized() {
    EgoStateProvenance prov;
    check(prov.summary() == "none", "empty provenance reads 'none'");
    check(!prov.any(), "empty provenance.any() false");
}

void test_map_never_emits_nan_for_finite_input() {
    const Ss2Entity ego = moving_ego();
    const auto s = map_ego_state(ego, "r", 1, SynthesizedDefaults{});
    const bool finite =
        std::isfinite(s.position.x) && std::isfinite(s.position.y) && std::isfinite(s.position.z) &&
        std::isfinite(s.orientation_rpy.x) && std::isfinite(s.orientation_rpy.y) &&
        std::isfinite(s.orientation_rpy.z) && std::isfinite(s.velocity.x) &&
        std::isfinite(s.acceleration.x) && std::isfinite(s.yaw_rate) &&
        std::isfinite(s.steering_angle);
    check(finite, "finite input -> finite state everywhere (NaN means fault downstream)");
}

}  // namespace

int main() {
    test_quaternion_identity();
    test_quaternion_roundtrip();
    test_quaternion_yaw_sign();
    test_quaternion_unnormalized();
    test_quaternion_degenerate_is_not_nan();
    test_quaternion_gimbal_lock_is_not_nan();

    test_select_ego_by_type();
    test_select_ego_by_name();
    test_select_ego_named_but_absent_does_not_fall_back();
    test_select_ego_empty();

    test_time_conversion();

    test_map_passthrough_fields();
    test_map_declares_synthesized_fields();
    test_provenance_summary_when_nothing_synthesized();
    test_map_never_emits_nan_for_finite_input();

    std::printf("%s  %d checks, %d failures\n",
                g_failures == 0 ? "PASS" : "FAIL", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
