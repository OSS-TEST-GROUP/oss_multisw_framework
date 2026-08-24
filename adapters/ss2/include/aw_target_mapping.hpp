// aw_target_mapping -- real Autoware control command -> canonical SiLS ControlTarget.
// Stage 3, step 5 (vECU in the vehicle_interface seat). Docs:
// docs/sils/sils-stage3-autoware-partition.md, docs/sils/sils-vehicle-interface-autoware.md
//
// This is the INBOUND half of the Autoware anti-corruption layer and the mirror image of
// ss2_control_mapping.hpp, which carries our command OUT to Autoware's topics.  Both
// directions live in headers with no ROS 2 and no DDS include so the arithmetic -- the part
// that can be silently wrong (a sign, a gear code, a unit) -- is testable on its own.
//
//   ss2_control_mapping.hpp   canonical VehicleControl  ──▶  Control + GearCommand   (out)
//   aw_target_mapping.hpp     Control + GearCommand     ──▶  canonical ControlTarget (in)
//
// WHY THE FIELDS ARE 1:1 AND STILL NEED A LAYER
//
// idl/dds/sils/ControlTarget.idl was written as an Autoware mirror, so steering_tire_angle
// and longitudinal.acceleration are already rad and m/s^2 and pass through unscaled.  The
// layer is not here for unit conversion; it is here because the vECU is a bare DDS
// participant that must never match a ROS-mangled topic (`rt/...` + `pkg::msg::dds_::T_`).
// See ss2_ego_bridge.cpp's BR-C-2 note.

#ifndef SILS_AW_TARGET_MAPPING_HPP_
#define SILS_AW_TARGET_MAPPING_HPP_

#include <cstdint>
#include <string>

#include "ss2_control_mapping.hpp"  // CanonicalGear, AutowareGear -- one source of truth

namespace sils {
namespace aw {

using sils::ss2::AutowareGear;
using sils::ss2::CanonicalGear;

// What we read off the two ROS topics, still ROS-free.
struct AutowareControlSample {
    std::int64_t stamp_ns = 0;
    double steering_tire_angle = 0.0;        // Control.lateral.steering_tire_angle, rad
    double longitudinal_acceleration = 0.0;  // Control.longitudinal.acceleration, m/s^2
    AutowareGear gear = AutowareGear::kNone; // latest GearCommand.command, kNone until one arrives
};

// Mirror of idl/dds/sils/ControlTarget.idl, DDS-free.
struct CanonicalControlTarget {
    std::string run_id;
    std::uint64_t seq = 0;
    std::int64_t stamp_ns = 0;
    double steering_angle = 0.0;
    double acceleration = 0.0;
    std::uint8_t gear = static_cast<std::uint8_t>(CanonicalGear::kNeutral);
    bool gear_recognized = true;  // false => Autoware sent NONE or something we do not model
};

// --- gear --------------------------------------------------------------------
//
// The inverse of ss2_control_mapping.hpp's map_gear(), and it inherits that function's
// safety rule: AN UNRECOGNISED GEAR MUST NOT BECOME DRIVE.
//
// The reasoning is the same one written out there, pointed the other way.  Downstream of
// this mapping the canonical gear reaches the plant through the vECU, where a forward gear
// lets commanded acceleration move the car.  If a gear byte we failed to parse defaulted to
// kDrive, a rig could move the vehicle on input it never actually understood -- a PASS the
// SUT did not earn.  kNeutral coasts instead, and the caller counts it.
//
// kNeutral rather than kPark because coasting is the smaller lie: PARK asserts a
// commanded state Autoware never asked for, and on a plant that models it that is a brake.
inline std::uint8_t map_gear_to_canonical(AutowareGear g, bool* recognized = nullptr) {
    if (recognized != nullptr) *recognized = true;
    switch (g) {
        case AutowareGear::kPark:    return static_cast<std::uint8_t>(CanonicalGear::kPark);
        case AutowareGear::kReverse: return static_cast<std::uint8_t>(CanonicalGear::kReverse);
        case AutowareGear::kNeutral: return static_cast<std::uint8_t>(CanonicalGear::kNeutral);
        case AutowareGear::kDrive:   return static_cast<std::uint8_t>(CanonicalGear::kDrive);
        case AutowareGear::kNone:    break;
    }
    if (recognized != nullptr) *recognized = false;
    return static_cast<std::uint8_t>(CanonicalGear::kNeutral);
}

// --- the target ---------------------------------------------------------------
//
// ★ ACCELERATION IS NOT SIGN-FLIPPED, for the same reason it is not flipped on the way out.
//
//   Autoware's Control.longitudinal.acceleration is signed along the direction of travel,
//   and so is our canonical field (see ControlTarget.idl and the long note in
//   ss2_control_mapping.hpp).  The conventions already agree in both gears.  A flip or an
//   abs() added here would break REVERSE and leave DRIVE working -- i.e. it would pass
//   every test this rig currently runs.  That is why it is written down.
//
// ★ Control.longitudinal.velocity IS DROPPED, and that is deliberate.
//
//   The canonical contract pins the longitudinal mode to acceleration; ControlTarget.idl
//   has no velocity field at all.  Autoware does publish one, and on the measured stack it
//   is the trajectory's target speed.  Carrying it would mean widening the canonical
//   contract, so the loss is recorded here rather than papered over: a vECU behaviour that
//   wants to track a speed target cannot get it from this path today.
inline CanonicalControlTarget map_control_target(const AutowareControlSample& c,
                                                 const std::string& run_id,
                                                 std::uint64_t seq) {
    CanonicalControlTarget out;
    out.run_id = run_id;
    out.seq = seq;
    out.stamp_ns = c.stamp_ns;
    out.steering_angle = c.steering_tire_angle;      // both rad, both front road-wheel: 1:1
    out.acceleration = c.longitudinal_acceleration;  // both m/s^2, both travel-signed: 1:1
    out.gear = map_gear_to_canonical(c.gear, &out.gear_recognized);
    return out;
}

}  // namespace aw
}  // namespace sils

#endif  // SILS_AW_TARGET_MAPPING_HPP_
