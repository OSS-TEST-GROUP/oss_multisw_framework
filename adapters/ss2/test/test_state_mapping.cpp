// Unit tests for aw_state_mapping.hpp -- the orientation arithmetic on the Autoware
// vehicle-state path.
//
// Dependency-free (no gtest, no ROS, no DDS) so it runs anywhere:
//     c++ -std=c++20 -I../include test_state_mapping.cpp -o t && ./t
// or via adapters/ss2/run_tests.sh.
//
// WHY THIS FILE EXISTS
//
// Two bridges now sit on opposite ends of the same rotation:
//
//   aw_ego_state_bridge      ADAPI quaternion  -> EgoState.orientation_rpy   quat_to_rpy
//   aw_localization_bridge   EgoState RPY      -> nav_msgs/Odometry pose     rpy_to_quat
//
// If the two conventions disagree, the ego's heading comes back rotated and NOTHING
// reports an error -- planning simply steers toward a goal that is not where it thinks.
// A round trip is the only cheap way to pin that down, so that is most of what is here.
//
// The other two traps, both of which produce a silent NaN rather than a crash:
//   1. asin() past ±1 at pitch = ±90°, from ordinary floating-point error
//   2. a default-constructed quaternion of all zeros, which is not a rotation at all

#include "aw_state_mapping.hpp"

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

using sils::aw::Quat;
using sils::aw::quat_to_rpy;
using sils::aw::rpy_to_quat;
using sils::aw::Vec3;

constexpr double kPi = 3.14159265358979323846;
constexpr double kTol = 1e-12;

// --- the round trip ------------------------------------------------------------
//
// The one that matters. Every angle triple below survives RPY -> quat -> RPY unchanged.
void test_round_trip_preserves_rpy() {
    struct Case { double r, p, y; const char* name; };
    const Case cases[] = {
        {0.0, 0.0, 0.0, "identity"},
        {0.0, 0.0, kPi / 2, "yaw +90"},
        {0.0, 0.0, -kPi / 2, "yaw -90"},
        {0.0, 0.0, 2.0, "yaw 2 rad (a real map heading)"},
        {0.1, 0.0, 0.0, "roll only"},
        {0.0, 0.2, 0.0, "pitch only"},
        {0.05, -0.12, 1.83, "all three, road-realistic"},
        {-0.4, 0.3, -2.7, "all three, large"},
    };
    for (const Case& c : cases) {
        const Quat q = rpy_to_quat(c.r, c.p, c.y);
        const Vec3 back = quat_to_rpy(q.x, q.y, q.z, q.w);
        check_near(back.x, c.r, 1e-9, std::string("round trip roll: ") + c.name);
        check_near(back.y, c.p, 1e-9, std::string("round trip pitch: ") + c.name);
        check_near(back.z, c.y, 1e-9, std::string("round trip yaw: ") + c.name);
    }
}

// Yaw near ±180° wraps: atan2 returns -pi where +pi went in. The ROTATION is identical,
// which is what consumers act on, so compare the quaternions rather than the angles.
void test_round_trip_at_the_yaw_wrap() {
    const Quat a = rpy_to_quat(0.0, 0.0, kPi);
    const Vec3 back = quat_to_rpy(a.x, a.y, a.z, a.w);
    check_near(std::abs(back.z), kPi, 1e-9, "yaw 180 comes back as ±pi");
    const Quat b = rpy_to_quat(0.0, 0.0, back.z);
    // q and -q are the same rotation; compare on whichever sign w carries.
    const double s = (a.w * b.w < 0.0) ? -1.0 : 1.0;
    check_near(s * b.x, a.x, 1e-9, "yaw wrap: same rotation x");
    check_near(s * b.y, a.y, 1e-9, "yaw wrap: same rotation y");
    check_near(s * b.z, a.z, 1e-9, "yaw wrap: same rotation z");
    check_near(s * b.w, a.w, 1e-9, "yaw wrap: same rotation w");
}

// --- the two silent-NaN traps ---------------------------------------------------

void test_default_quaternion_is_a_real_rotation() {
    const Quat q;
    check_near(q.w, 1.0, 0, "default Quat has w=1, not an all-zero non-rotation");
    const Vec3 rpy = quat_to_rpy(q.x, q.y, q.z, q.w);
    check(!std::isnan(rpy.z), "default Quat does not decode to NaN yaw");
    check_near(rpy.x, 0.0, kTol, "default Quat is zero roll");
    check_near(rpy.y, 0.0, kTol, "default Quat is zero pitch");
    check_near(rpy.z, 0.0, kTol, "default Quat is zero yaw");
}

void test_gimbal_pitch_does_not_produce_nan() {
    // Straight up. Rounding pushes sinp a few ulps past 1.0 here; without the clamp in
    // quat_to_rpy this returns NaN and every downstream steering term becomes NaN.
    const Quat q = rpy_to_quat(0.0, kPi / 2, 0.0);
    const Vec3 rpy = quat_to_rpy(q.x, q.y, q.z, q.w);
    check(!std::isnan(rpy.x) && !std::isnan(rpy.y) && !std::isnan(rpy.z),
          "pitch +90 decodes without NaN");
    check_near(rpy.y, kPi / 2, 1e-9, "pitch +90 comes back as +90");

    const Quat qd = rpy_to_quat(0.0, -kPi / 2, 0.0);
    const Vec3 rpyd = quat_to_rpy(qd.x, qd.y, qd.z, qd.w);
    check(!std::isnan(rpyd.y), "pitch -90 decodes without NaN");
    check_near(rpyd.y, -kPi / 2, 1e-9, "pitch -90 comes back as -90");
}

// --- basic sanity ---------------------------------------------------------------

void test_quaternions_are_unit_length() {
    const Quat q = rpy_to_quat(0.3, -0.7, 2.1);
    const double n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    check_near(n, 1.0, 1e-12, "rpy_to_quat returns a unit quaternion");
}

// Yaw-only is the overwhelmingly common case on a road vehicle, and it has a closed form
// worth pinning: q = (0, 0, sin(yaw/2), cos(yaw/2)). If someone reorders the ZYX product
// the round trip still passes for small angles but this does not.
void test_yaw_only_has_the_known_closed_form() {
    const double yaw = 1.234;
    const Quat q = rpy_to_quat(0.0, 0.0, yaw);
    check_near(q.x, 0.0, kTol, "yaw-only: x is 0");
    check_near(q.y, 0.0, kTol, "yaw-only: y is 0");
    check_near(q.z, std::sin(yaw / 2), kTol, "yaw-only: z is sin(yaw/2)");
    check_near(q.w, std::cos(yaw / 2), kTol, "yaw-only: w is cos(yaw/2)");
}

}  // namespace

int main() {
    test_round_trip_preserves_rpy();
    test_round_trip_at_the_yaw_wrap();
    test_default_quaternion_is_a_real_rotation();
    test_gimbal_pitch_does_not_produce_nan();
    test_quaternions_are_unit_length();
    test_yaw_only_has_the_known_closed_form();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
