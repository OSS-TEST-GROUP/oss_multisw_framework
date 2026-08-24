// Unit tests for the canonical VehicleControl -> Autoware command mapping (B2-3).
//
// Dependency-free (no gtest, no ROS, no DDS) so it runs anywhere:
//     c++ -std=c++20 -I../include test_control_mapping.cpp -o t && ./t
// or via adapters/ss2/run_tests.sh.
//
// This mapping is four assignments long, which is exactly why it needs tests: there is
// nothing here that LOOKS dangerous. The three places it can be silently wrong:
//
//   1. the acceleration sign in REVERSE -- the simulator multiplies by a gear sign, so
//      "obviously correct" code that takes abs() or flips passes every DRIVE test
//   2. the gear default for an unrecognised byte -- defaulting to DRIVE makes garbage
//      input drive the car
//   3. what happens when the vECU stops publishing -- the simulator keeps applying the
//      last command forever

#include "ss2_control_mapping.hpp"

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

using sils::ss2::AutowareCommand;
using sils::ss2::AutowareGear;
using sils::ss2::CanonicalGear;
using sils::ss2::CanonicalVehicleControl;
using sils::ss2::map_control;
using sils::ss2::map_gear;
using sils::ss2::withdraw_command;

CanonicalVehicleControl Cmd(double steer, double accel, CanonicalGear gear) {
    CanonicalVehicleControl c;
    c.run_id = "t";
    c.seq = 7;
    c.stamp_ns = 1'234'567'890;
    c.steering_angle = steer;
    c.acceleration = accel;
    c.gear = static_cast<std::uint8_t>(gear);
    return c;
}

// The simulator's arithmetic, transcribed. Tests assert on what the CAR does, not on what
// the message says -- a mapping test that only compares struct fields to themselves cannot
// catch a convention error, because both sides of the comparison share the mistake.
//   sign_of:  concealer/src/autoware_universe.cpp
//   input(0): simple_sensor_simulator/.../ego_entity_simulation.cpp:321
double SimGearSign(AutowareGear g) {
    switch (g) {
        case AutowareGear::kReverse: return -1.0;
        case AutowareGear::kNone:    return 0.0;
        default:                     return 1.0;  // DRIVE, NEUTRAL, PARK
    }
}
double SimBodyAcceleration(const AutowareCommand& c) {
    return SimGearSign(c.gear) * c.longitudinal_acceleration;
}

// --- pass-through ------------------------------------------------------------

void test_units_are_not_converted() {
    // Both sides are rad and m/s^2. A "helpful" deg->rad or km/h->m/s inserted here would
    // be invisible in review precisely because the ego adapter's README warns that SS_v2
    // needs no conversions -- someone may assume this direction does.
    const AutowareCommand a = map_control(Cmd(0.25, -1.5, CanonicalGear::kDrive));
    check_near(a.steering_tire_angle, 0.25, 0, "steering passes through unscaled");
    check_near(a.longitudinal_acceleration, -1.5, 0, "acceleration passes through unscaled");
    check(a.stamp_ns == 1'234'567'890, "stamp passes through");
}

void test_velocity_is_zero_and_that_is_deliberate() {
    // Canonical VehicleControl has no velocity field. Publishing 0.0 is the honest answer;
    // this test exists so that anyone who later "fixes" it by inventing a velocity has to
    // confront the decision instead of sliding past it. See map_control's §velocity.
    const AutowareCommand a = map_control(Cmd(0.0, 3.0, CanonicalGear::kDrive));
    check_near(a.longitudinal_velocity, 0.0, 0, "velocity is 0.0 (no canonical source)");
}

// --- the sign, in both gears -------------------------------------------------

void test_drive_acceleration_moves_the_car_forward() {
    const AutowareCommand a = map_control(Cmd(0.0, 2.0, CanonicalGear::kDrive));
    check_near(SimBodyAcceleration(a), 2.0, 1e-12,
               "DRIVE + positive accel -> forward body acceleration");
}

void test_drive_braking_decelerates() {
    const AutowareCommand a = map_control(Cmd(0.0, -2.0, CanonicalGear::kDrive));
    check_near(SimBodyAcceleration(a), -2.0, 1e-12,
               "DRIVE + negative accel -> rearward body acceleration (braking)");
}

// ★ The two that a sign flip or an abs() would break, and nothing else would.
void test_reverse_acceleration_speeds_the_car_up_backwards() {
    const AutowareCommand a = map_control(Cmd(0.0, 2.0, CanonicalGear::kReverse));
    check_near(SimBodyAcceleration(a), -2.0, 1e-12,
               "REVERSE + positive accel -> BACKWARD body acceleration (speeding up in reverse)");
}

void test_reverse_braking_slows_the_car_down() {
    const AutowareCommand a = map_control(Cmd(0.0, -2.0, CanonicalGear::kReverse));
    check_near(SimBodyAcceleration(a), 2.0, 1e-12,
               "REVERSE + negative accel -> FORWARD body acceleration (braking while reversing)");
}

// --- gear --------------------------------------------------------------------

void test_gear_codes_map_one_for_one() {
    check(map_gear(0) == AutowareGear::kPark, "0 -> PARK");
    check(map_gear(1) == AutowareGear::kReverse, "1 -> REVERSE");
    check(map_gear(2) == AutowareGear::kNeutral, "2 -> NEUTRAL");
    check(map_gear(3) == AutowareGear::kDrive, "3 -> DRIVE");
}

void test_unknown_gear_coasts_rather_than_drives() {
    bool recognized = true;
    const AutowareGear g = map_gear(200, &recognized);
    check(!recognized, "an out-of-range gear byte is reported as unrecognised");
    check(g == AutowareGear::kNone, "unrecognised gear maps to NONE, not DRIVE");

    // And NONE must actually stop the car, which is the whole reason for the choice.
    AutowareCommand a = map_control(Cmd(0.0, 5.0, static_cast<CanonicalGear>(200)));
    check_near(SimBodyAcceleration(a), 0.0, 1e-12,
               "a garbage gear zeroes the commanded acceleration -- garbage never drives");
}

void test_recognized_flag_is_true_for_valid_gears() {
    const AutowareCommand a = map_control(Cmd(0.0, 1.0, CanonicalGear::kDrive));
    check(a.gear_recognized, "a valid gear is not flagged as unrecognised");
}

// --- the vECU going quiet ----------------------------------------------------

void test_withdrawing_a_command_stops_commanding_acceleration() {
    AutowareCommand last;
    last.steering_tire_angle = 0.3;
    last.longitudinal_acceleration = 2.5;
    last.gear = AutowareGear::kDrive;

    const AutowareCommand w = withdraw_command(last);
    check_near(w.longitudinal_acceleration, 0.0, 0, "withdrawn command has zero acceleration");
    check_near(SimBodyAcceleration(w), 0.0, 1e-12, "so the car stops being driven");
}

void test_withdrawing_holds_steering_and_gear() {
    // Centring the wheel would be issuing a command, and the point is to stop commanding.
    AutowareCommand last;
    last.steering_tire_angle = 0.3;
    last.longitudinal_acceleration = 2.5;
    last.gear = AutowareGear::kReverse;

    const AutowareCommand w = withdraw_command(last);
    check_near(w.steering_tire_angle, 0.3, 0, "steering is held, not centred");
    check(w.gear == AutowareGear::kReverse, "gear is held");
}

void test_withdrawing_is_idempotent() {
    // The bridge republishes the withdrawal on a timer while the vECU stays quiet.
    AutowareCommand last;
    last.longitudinal_acceleration = 2.5;
    last.steering_tire_angle = -0.1;
    const AutowareCommand once = withdraw_command(last);
    const AutowareCommand twice = withdraw_command(once);
    check_near(twice.longitudinal_acceleration, 0.0, 0, "withdraw twice is still zero");
    check_near(twice.steering_tire_angle, -0.1, 0, "withdraw twice still holds steering");
}

}  // namespace

int main() {
    test_units_are_not_converted();
    test_velocity_is_zero_and_that_is_deliberate();
    test_drive_acceleration_moves_the_car_forward();
    test_drive_braking_decelerates();
    test_reverse_acceleration_speeds_the_car_up_backwards();
    test_reverse_braking_slows_the_car_down();
    test_gear_codes_map_one_for_one();
    test_unknown_gear_coasts_rather_than_drives();
    test_recognized_flag_is_true_for_valid_gears();
    test_withdrawing_a_command_stops_commanding_acceleration();
    test_withdrawing_holds_steering_and_gear();
    test_withdrawing_is_idempotent();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
