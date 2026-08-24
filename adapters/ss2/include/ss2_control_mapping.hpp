// Canonical VehicleControl -> Autoware control command.  Loop B, work item B2-3.
//
// PURE: no ROS 2, no DDS, no simulator.  Same reason as ss2_ego_mapping.hpp -- everything
// here that can be silently wrong (a sign, a gear code, a unit) is arithmetic, and
// arithmetic should be checkable on a laptop.  src/ss2_control_bridge.cpp does the wiring.
//
// DIRECTION: this is the RETURN path.  ss2_ego_mapping.hpp carries the simulator's state
// out to the vECU; this carries the vECU's command back in, closing the loop.
//
//     vECU ──VehicleControl(DDS)──▶ [bridge] ──/control/command/control_cmd──▶ concealer
//                                             ──/control/command/gear_cmd────▶ AutowareUniverse
//                                                                                    │
//                                                                          vehicle model moves ego
//
// The two topics are not a choice.  concealer::AutowareUniverse subscribes to exactly
// those names (external/concealer/src/autoware_universe.cpp, pin 3370c817) and reads
// exactly three numbers out of them -- see map_control below.

#ifndef SS2_CONTROL_MAPPING_HPP_
#define SS2_CONTROL_MAPPING_HPP_

#include <cstdint>
#include <string>

namespace sils {
namespace ss2 {

// The canonical gear octet, shared by idl/dds/sils/EgoState.idl and VehicleControl.idl.
// Kept as named values here so the mapping below reads as a mapping and not as a table of
// magic numbers.
enum class CanonicalGear : std::uint8_t {
    kPark = 0,
    kReverse = 1,
    kNeutral = 2,
    kDrive = 3,
};

// The subset of autoware_vehicle_msgs/msg/GearCommand this adapter ever emits.
//
// NAMED, not numeric, ON PURPOSE.  The real constants are static members of the generated
// message class, and the bridge resolves them BY NAME (GearCommand::DRIVE, ...).  That
// keeps the same rule the Autoware stub learned the hard way: a hardcoded integer produces
// a message that is structurally valid and means something else, and nothing catches it.
// Here a renamed or removed constant is a compile error instead.
enum class AutowareGear {
    kNone,
    kNeutral,
    kDrive,
    kReverse,
    kPark,
};

// Mirror of idl/dds/sils/VehicleControl.idl, DDS-free.
struct CanonicalVehicleControl {
    std::string run_id;
    std::uint64_t seq = 0;
    std::int64_t stamp_ns = 0;
    double steering_angle = 0.0;   // front road-wheel angle, rad
    double acceleration = 0.0;     // m/s^2, signed along the direction of travel
    std::uint8_t gear = static_cast<std::uint8_t>(CanonicalGear::kDrive);
};

// What goes on the two ROS topics, still ROS-free.
struct AutowareCommand {
    std::int64_t stamp_ns = 0;
    double steering_tire_angle = 0.0;        // Control.lateral.steering_tire_angle
    double longitudinal_velocity = 0.0;      // Control.longitudinal.velocity   -- see §velocity
    double longitudinal_acceleration = 0.0;  // Control.longitudinal.acceleration
    AutowareGear gear = AutowareGear::kNone; // GearCommand.command
    bool gear_recognized = true;             // false => the canonical octet was out of range
};

// --- gear --------------------------------------------------------------------
//
// An UNRECOGNISED gear maps to kNone, not to kDrive.
//
// That choice matters because of how the simulator uses it:
//
//     sign_of(NONE) == 0        (concealer/src/autoware_universe.cpp)
//     input(0) = gear_sign * acceleration + slope
//               (simple_sensor_simulator/.../ego_entity_simulation.cpp:321)
//
// so kNone zeroes the commanded acceleration and the ego coasts.  Defaulting to kDrive
// would instead make a garbage gear byte drive the car -- and a test rig that moves the
// vehicle on unparsed input can report a PASS the SUT did not earn.  Coast, and count it.
inline AutowareGear map_gear(std::uint8_t canonical, bool* recognized = nullptr) {
    if (recognized != nullptr) *recognized = true;
    switch (static_cast<CanonicalGear>(canonical)) {
        case CanonicalGear::kPark:    return AutowareGear::kPark;
        case CanonicalGear::kReverse: return AutowareGear::kReverse;
        case CanonicalGear::kNeutral: return AutowareGear::kNeutral;
        case CanonicalGear::kDrive:   return AutowareGear::kDrive;
    }
    if (recognized != nullptr) *recognized = false;
    return AutowareGear::kNone;
}

// --- the command -------------------------------------------------------------
//
// ★ ACCELERATION PASSES THROUGH UNCHANGED, and that is NOT obvious.  It looks like it
//   needs a sign flip, so here is why it does not.
//
//   The simulator computes            input(0) = gear_sign * acceleration
//   with gear_sign = -1 in REVERSE.  input(0) is longitudinal acceleration in the VEHICLE
//   BODY frame (forward positive).  Our canonical value is signed along the DIRECTION OF
//   TRAVEL (+ = speed up, - = brake), whichever way the gear points.  Work the reverse
//   case through:
//
//     canonical +a in REVERSE  ->  input(0) = -a  ->  body-frame acceleration is backward
//                                                 ->  the car speeds up going backwards  ✓
//     canonical -a in REVERSE  ->  input(0) = +a  ->  body-frame acceleration is forward
//                                                 ->  the car slows down going backwards  ✓
//
//   The two conventions already agree.  Adding an abs() or a flip here -- which a reviewer
//   might well suggest, because concealer's own TODO calls its value "unsigned" -- would
//   break reverse and leave DRIVE working, i.e. it would pass every test we currently run.
//   That is why this reasoning is written down instead of assumed, and why the reverse
//   cases are in test_control_mapping.cpp.
//
// ★ VELOCITY IS 0.0 BECAUSE WE HAVE NONE, and that is a trap worth knowing about.
//
//   The canonical VehicleControl contract carries acceleration only -- the IDL pins the
//   longitudinal mode to acceleration deliberately.  Control.longitudinal.velocity is
//   ignored by every *_STEER_ACC* vehicle model, which is what the ego uses by default.
//   But *_STEER_VEL models feed it straight in as the commanded speed, where 0.0 means
//   STOP.  So on a VEL-configured ego this adapter silently parks the car.  The bridge
//   says so at startup; there is nothing better to publish, since inventing a velocity
//   would be fabricating a command the vECU never gave.
inline AutowareCommand map_control(const CanonicalVehicleControl& c) {
    AutowareCommand out;
    out.stamp_ns = c.stamp_ns;
    out.steering_tire_angle = c.steering_angle;  // both rad, both front road-wheel: 1:1
    out.longitudinal_acceleration = c.acceleration;
    out.longitudinal_velocity = 0.0;
    out.gear = map_gear(c.gear, &out.gear_recognized);
    return out;
}

// --- what to send when the vECU goes quiet -----------------------------------
//
// concealer RETAINS the last command it received and hands it to the vehicle model on
// every frame, forever.  So a vECU that crashes mid-scenario does not stop the ego: the
// last acceleration keeps being applied and the car drives off on its own.  It can even
// reach the goal, and the run would be scored a PASS that nothing earned -- the same
// failure shape as a golden captured from synthesized fields.
//
// Withdrawing the command means zeroing the acceleration.  Steering and gear are HELD:
// snapping the wheel to centre would be issuing a command of our own, and we are trying to
// stop commanding.  With acceleration 0 the model coasts down through drag, which is the
// honest physical reading of "nobody is driving any more".
inline AutowareCommand withdraw_command(const AutowareCommand& last) {
    AutowareCommand out = last;
    out.longitudinal_acceleration = 0.0;
    out.longitudinal_velocity = 0.0;
    return out;
}

}  // namespace ss2
}  // namespace sils

#endif  // SS2_CONTROL_MAPPING_HPP_
