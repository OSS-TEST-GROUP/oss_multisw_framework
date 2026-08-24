// aw_state_mapping -- real Autoware ADAPI vehicle state -> canonical SiLS EgoState.
// Stage 3, step 5. Docs: docs/sils/sils-stage3-autoware-partition.md
//
// The inbound STATE half, sibling of aw_target_mapping.hpp (the inbound COMMAND half).
// ROS-free and DDS-free so the arithmetic is testable on its own -- and here there IS
// arithmetic, unlike the command path: a quaternion has to become roll/pitch/yaw.
//
// ★ WHY ADAPI AND NOT /vehicle/status/* + /localization/*
//
// Autoware exposes the same information twice: spread over five raw topics
// (/localization/kinematic_state, /localization/acceleration,
// /vehicle/status/{velocity,steering,gear}_status) or consolidated into two ADAPI topics
// (/api/vehicle/kinematics, /api/vehicle/status). ADAPI is the stable, versioned surface --
// it is what `vehicle_info_mgr`'s DDSProvider reads too -- and two subscriptions with one
// timestamp each beat five that have to be joined. Use ADAPI.
//
// ★ THIS IS A STAND-IN, NOT THE TARGET WIRING
//
// In the target picture the ego state comes from the PLANT (MORAI / SS_v2), and Autoware is
// a CONSUMER of vehicle status, not a producer of ground truth. What this bridge reads is
// Autoware's own simple_planning_simulator reporting on itself. That is fine for developing
// and exercising an ECU against real Autoware, and it is NOT a closed loop: an ECU fed from
// here cannot influence what it reads. Say so in any report that uses it.

#ifndef SILS_AW_STATE_MAPPING_HPP_
#define SILS_AW_STATE_MAPPING_HPP_

#include <cmath>
#include <cstdint>
#include <string>

#include "ss2_control_mapping.hpp"  // CanonicalGear -- one source of truth for the octet

namespace sils {
namespace aw {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

// The subset of autoware_adapi_v1_msgs Gear constants we map. Named, not numeric, for the
// same reason as everywhere else in this tree: a renamed constant must be a compile error,
// not a wrong byte. Resolution happens in the .cpp, which has the generated message class.
enum class AdapiGear {
    kUnknown,
    kNeutral,
    kDrive,
    kReverse,
    kPark,
    kLow,
};

// What we read off the two ADAPI topics, still ROS-free.
struct AutowareStateSample {
    std::int64_t stamp_ns = 0;
    Vec3 position;                 // map frame, m
    double qx = 0.0, qy = 0.0, qz = 0.0, qw = 1.0;  // map frame orientation
    Vec3 twist_linear;             // BODY frame, m/s   (ADAPI twist is body-frame)
    Vec3 twist_angular;            // body frame, rad/s
    Vec3 accel_linear;             // body frame, m/s^2
    double steering_tire_angle = 0.0;  // rad, front road-wheel
    AdapiGear gear = AdapiGear::kUnknown;
    bool have_status = false;      // false until the first /api/vehicle/status arrives
};

// Mirror of idl/dds/sils/EgoState.idl, DDS-free.
struct CanonicalEgoState {
    std::string run_id;
    std::uint64_t seq = 0;
    std::int64_t stamp_ns = 0;
    Vec3 position;
    Vec3 orientation_rpy;
    Vec3 velocity;
    Vec3 acceleration;
    double yaw_rate = 0.0;
    double steering_angle = 0.0;
    std::uint8_t gear = static_cast<std::uint8_t>(sils::ss2::CanonicalGear::kNeutral);
    bool gear_known = true;   // false => ADAPI said UNKNOWN, or no status seen yet
};

// --- orientation --------------------------------------------------------------
//
// Quaternion -> roll/pitch/yaw (ZYX / aerospace convention), which is what
// EgoState.orientation_rpy declares. Standard formulation; the only subtlety is the pitch
// branch: asin() of a value that floating-point error can push just past ±1 returns NaN,
// and a NaN yaw would propagate silently into every consumer's steering arithmetic. Clamp.
inline Vec3 quat_to_rpy(double x, double y, double z, double w) {
    Vec3 rpy;
    const double sinr_cosp = 2.0 * (w * x + y * z);
    const double cosr_cosp = 1.0 - 2.0 * (x * x + y * y);
    rpy.x = std::atan2(sinr_cosp, cosr_cosp);

    double sinp = 2.0 * (w * y - z * x);
    if (sinp > 1.0) sinp = 1.0;
    if (sinp < -1.0) sinp = -1.0;
    rpy.y = std::asin(sinp);

    const double siny_cosp = 2.0 * (w * z + x * y);
    const double cosy_cosp = 1.0 - 2.0 * (y * y + z * z);
    rpy.z = std::atan2(siny_cosp, cosy_cosp);
    return rpy;
}

// A quaternion, x/y/z/w, in the order geometry_msgs declares them.
struct Quat {
    double x = 0.0, y = 0.0, z = 0.0, w = 1.0;
};

// roll/pitch/yaw (ZYX) -> quaternion. The exact inverse of quat_to_rpy above, and the
// direction aw_localization_bridge needs: EgoState carries RPY, nav_msgs/Odometry wants a
// quaternion.
//
// The identity is w=1 rather than all-zeros. An all-zero quaternion is not a rotation at
// all, and Autoware's tf2 conversions turn one into NaN heading rather than rejecting it --
// so a default-constructed Quat must already be valid.
inline Quat rpy_to_quat(double roll, double pitch, double yaw) {
    const double cr = std::cos(roll * 0.5), sr = std::sin(roll * 0.5);
    const double cp = std::cos(pitch * 0.5), sp = std::sin(pitch * 0.5);
    const double cy = std::cos(yaw * 0.5), sy = std::sin(yaw * 0.5);
    Quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

// --- gear ---------------------------------------------------------------------
//
// LOW is a forward gear, so it maps to kDrive. UNKNOWN maps to kNeutral and sets
// gear_known=false -- the same "coast, and count it" rule the command path uses. A state
// consumer that treats an unparsed gear as DRIVE would compute a direction of travel the
// vehicle never reported.
inline std::uint8_t map_gear(AdapiGear g, bool* known = nullptr) {
    using CG = sils::ss2::CanonicalGear;
    if (known != nullptr) *known = true;
    switch (g) {
        case AdapiGear::kPark:    return static_cast<std::uint8_t>(CG::kPark);
        case AdapiGear::kReverse: return static_cast<std::uint8_t>(CG::kReverse);
        case AdapiGear::kNeutral: return static_cast<std::uint8_t>(CG::kNeutral);
        case AdapiGear::kDrive:
        case AdapiGear::kLow:     return static_cast<std::uint8_t>(CG::kDrive);
        case AdapiGear::kUnknown: break;
    }
    if (known != nullptr) *known = false;
    return static_cast<std::uint8_t>(CG::kNeutral);
}

// --- the state ----------------------------------------------------------------
//
// ★ NO UNIT CONVERSION HAPPENS HERE, and that is worth stating because the neighbouring
//   VISS path DOES convert.
//
//   ADAPI is SI throughout: m, m/s, m/s^2, rad, rad/s -- the same units EgoState declares.
//   The VSS/VISS mapping in service/core/vehicle_info_mgr converts the very same fields to
//   km/h and degrees because the VSS catalogue declares them that way. Two consumers, two
//   unit systems, one source. Do not copy a conversion across from that file.
//
// ★ VELOCITY IS BODY-FRAME, matching EgoState's "vehicle frame" declaration. ADAPI's twist
//   is reported in base_link, so it passes through unrotated. If a future source reports
//   map-frame velocity it must be rotated HERE, not by the consumer.
inline CanonicalEgoState map_ego_state(const AutowareStateSample& s,
                                       const std::string& run_id,
                                       std::uint64_t seq) {
    CanonicalEgoState out;
    out.run_id = run_id;
    out.seq = seq;
    out.stamp_ns = s.stamp_ns;
    out.position = s.position;
    out.orientation_rpy = quat_to_rpy(s.qx, s.qy, s.qz, s.qw);
    out.velocity = s.twist_linear;
    out.acceleration = s.accel_linear;
    out.yaw_rate = s.twist_angular.z;
    out.steering_angle = s.steering_tire_angle;  // both rad, both road-wheel: 1:1
    out.gear = map_gear(s.have_status ? s.gear : AdapiGear::kUnknown, &out.gear_known);
    return out;
}

}  // namespace aw
}  // namespace sils

#endif  // SILS_AW_STATE_MAPPING_HPP_
