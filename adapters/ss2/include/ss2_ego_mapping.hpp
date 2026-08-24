// scenario_simulator_v2 -> canonical EgoState mapping.  (loop B, work item B2-1)
//
// THIS HEADER HAS NO ROS 2 AND NO DDS DEPENDENCY, on purpose.
//
// The adapter is an anti-corruption layer, and the part of it that can be wrong in a way
// nobody notices is the arithmetic -- frames, units, quaternion conventions. That part is
// isolated here as pure functions so it can be unit-tested on any host, without a ROS 2
// workspace, without a simulator, without a DDS domain.  src/ss2_ego_bridge.cpp does the
// transport and calls into this; test/test_mapping.cpp exercises this directly.
//
// Why that split matters here specifically: the sibling MORAI adapter has to convert
// deg->rad and km/h->m/s, and getting it wrong skews the loop by ~57.3x while every
// process still runs and every test still "passes" (idl/dds/sils/EgoState.idl).  SS_v2
// needs almost no unit conversion, which makes it MORE dangerous, not less -- there is
// nothing obviously scary to review, so an error in the quaternion or the ego selection
// would sail through.
//
// Field provenance is a first-class output (see EgoStateProvenance): SS_v2 does not
// publish everything canonical EgoState carries, and the adapter must say so rather than
// quietly emitting a plausible zero.
//
// Docs: docs/sils/sils-simulator-loops.md · docs/sils/sils-ss2-bringup.md
//       adapters/ss2/README.md (mapping table, measured source topic/QoS)

#ifndef SILS_ADAPTERS_SS2_EGO_MAPPING_HPP
#define SILS_ADAPTERS_SS2_EGO_MAPPING_HPP

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace sils::ss2 {

// --- input: the subset of traffic_simulator_msgs we actually read ------------
//
// Deliberately a flat POD rather than the ROS message: the mapping must be testable
// without traffic_simulator_msgs on the include path.  ss2_ego_bridge.cpp copies the
// ROS message into this, and that copy is the only place the ROS types appear.
//
// Source (verified against tier4/scenario_simulator_v2 master, 2026-08-07):
//   EntityStatusWithTrajectoryArray { EntityStatusWithTrajectory[] data }
//   EntityStatusWithTrajectory      { float64 time, string name, EntityStatus status, ... }
//   EntityStatus { EntityType type, EntitySubtype subtype, float64 time, string name,
//                  BoundingBox bounding_box, ActionStatus action_status,
//                  geometry_msgs/Pose pose, LaneletPose lanelet_pose, bool lanelet_pose_valid }
//   ActionStatus { string current_action, geometry_msgs/Twist twist,
//                  geometry_msgs/Accel accel, float64 linear_jerk }
//   EntityType   { uint8 EGO=0, VEHICLE=1, PEDESTRIAN=2, MISC_OBJECT=3; uint8 type }

enum class EntityType : std::uint8_t {
    kEgo = 0,
    kVehicle = 1,
    kPedestrian = 2,
    kMiscObject = 3,
};

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Quaternion {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;
};

struct Ss2Entity {
    std::string name;
    EntityType type = EntityType::kVehicle;
    double time_sec = 0.0;      // EntityStatus.time -- seconds on the simulation clock
    Vec3 position;              // pose.position, map frame, m
    Quaternion orientation;     // pose.orientation
    Vec3 twist_linear;          // action_status.twist.linear,  m/s
    Vec3 twist_angular;         // action_status.twist.angular, rad/s
    Vec3 accel_linear;          // action_status.accel.linear,  m/s^2
};

// --- output: canonical EgoState (idl/dds/sils/EgoState.idl) ------------------
// Plain mirror of the IDL struct so this header needs no generated types either.

struct CanonicalEgoState {
    std::string run_id;
    std::uint64_t seq = 0;
    std::int64_t stamp_ns = 0;
    Vec3 position;              // map frame, m
    Vec3 orientation_rpy;       // roll/pitch/yaw, rad
    Vec3 velocity;              // vehicle frame, m/s
    Vec3 acceleration;          // vehicle frame, m/s^2
    double yaw_rate = 0.0;      // rad/s
    double steering_angle = 0.0;// front road-wheel angle, rad
    std::uint8_t gear = 3;      // 0=PARK 1=REVERSE 2=NEUTRAL 3=DRIVE
};

// --- provenance --------------------------------------------------------------
//
// SS_v2's EntityStatus carries no steering angle and no gear.  Canonical EgoState
// requires both.  The adapter therefore SYNTHESIZES them, and this struct is how it
// admits that.
//
// Why not NaN for "unknown": NaN is already taken.  service/sils/vecu/src/
// vehicle_input_fault.hpp injects `state.steering_angle = nan` as a FAULT.  An adapter
// emitting NaN would be indistinguishable from an injected fault, and analyze_fault's
// completeness check (declared count == observed count) would then fail on a run where
// nothing was injected -- turning an honest "not available" into a spurious verdict.
//
// So: emit a documented constant, and carry the admission out-of-band.  The bridge logs
// this once at startup and once per run, and the run must use a plant id that marks it
// (see README "plant id" -- a golden captured here can never be compared against a run
// where these fields were real, because compare_golden's gate_plant() rejects it).
struct EgoStateProvenance {
    bool steering_angle_synthesized = false;
    bool gear_synthesized = false;

    // Human-readable one-liner for the startup banner / run log.
    std::string summary() const {
        std::string s;
        if (steering_angle_synthesized) s += "steering_angle";
        if (gear_synthesized) {
            if (!s.empty()) s += ", ";
            s += "gear";
        }
        return s.empty() ? std::string("none") : s;
    }

    bool any() const { return steering_angle_synthesized || gear_synthesized; }
};

// Values used for the fields SS_v2 does not provide.  Named so a reader of the trace can
// grep for why a steering angle is exactly 0.
struct SynthesizedDefaults {
    double steering_angle = 0.0;   // rad
    std::uint8_t gear = 3;         // DRIVE -- matches the vECU's own default (vehicle_behavior.hpp)
};

// --- ego selection -----------------------------------------------------------

// Pick the ego out of the entity array.
//
// Preference order:
//   1. explicit name match (`preferred_name` non-empty) -- scenarios can have more than
//      one EGO-typed entity, and "whichever came first" is not a contract
//   2. the first entity whose type is EGO
//
// Returns nullptr when there is no match.  The caller must treat that as "skip this
// sample", NOT as "publish zeros": an ego that has not spawned yet is normal during the
// first frames of a scenario, and emitting a zero state would look like a car parked at
// the map origin.
inline const Ss2Entity* select_ego(const std::vector<Ss2Entity>& entities,
                                   const std::string& preferred_name) {
    if (!preferred_name.empty()) {
        for (const auto& e : entities) {
            if (e.name == preferred_name) return &e;
        }
        return nullptr;   // named ego requested but absent -> do not silently fall back
    }
    for (const auto& e : entities) {
        if (e.type == EntityType::kEgo) return &e;
    }
    return nullptr;
}

// --- orientation -------------------------------------------------------------

// Quaternion -> roll/pitch/yaw (rad), intrinsic Z-Y-X (yaw, then pitch, then roll),
// which is the convention canonical EgoState.orientation_rpy is defined in and the one
// ROS uses (tf2::getEulerYPR).
//
// Pitch is clamped at +-pi/2 instead of letting asin() return NaN on a slightly
// out-of-range argument -- a quaternion that is off by 1e-16 from normalized is common
// after serialization, and a NaN here would propagate into the state as a fault-looking
// value (see EgoStateProvenance for why NaN is dangerous in this pipeline).
//
// At the gimbal-lock poles (|pitch| == pi/2) roll and yaw are not independently
// recoverable; the standard degenerate branch assigns the whole rotation to yaw and sets
// roll to 0.  A vehicle on a driving surface never reaches this, but a scenario that
// spawns an entity mid-flip would, and returning NaN there would be worse.
inline Vec3 quaternion_to_rpy(const Quaternion& q) {
    // Normalize defensively; an unnormalized quaternion silently scales the result.
    const double n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (!(n > 0.0)) return Vec3{};          // degenerate (all-zero) -> identity-ish
    const double x = q.x / n, y = q.y / n, z = q.z / n, w = q.w / n;

    Vec3 rpy;

    // pitch (y-axis)
    double sinp = 2.0 * (w * y - z * x);
    if (sinp > 1.0) sinp = 1.0;
    if (sinp < -1.0) sinp = -1.0;
    rpy.y = std::asin(sinp);

    constexpr double kGimbalEps = 1e-9;
    if (std::abs(std::abs(sinp) - 1.0) < kGimbalEps) {
        // Gimbal lock: fold roll into yaw.
        rpy.x = 0.0;
        rpy.z = 2.0 * std::atan2(x, w) * (sinp > 0.0 ? 1.0 : -1.0);
        return rpy;
    }

    // roll (x-axis)
    rpy.x = std::atan2(2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y));
    // yaw (z-axis)
    rpy.z = std::atan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z));
    return rpy;
}

// --- time --------------------------------------------------------------------

// EntityStatus.time is float64 SECONDS on the simulation clock; canonical stamp_ns is
// int64 NANOSECONDS.  Rounded, not truncated: at 30 Hz a truncating conversion biases
// every stamp downward by up to 1 ns, which is invisible per-sample but shows up as a
// systematic drift when two runs are diffed against each other.
//
// Negative times are passed through rather than clamped -- SS_v2 has no negative sim
// time today, but silently rewriting an out-of-contract input to 0 would hide a real
// upstream change.
inline std::int64_t sim_seconds_to_ns(double seconds) {
    return static_cast<std::int64_t>(std::llround(seconds * 1e9));
}

// ★ NaN IS A STATE, NOT AN ERROR -- and it must never reach sim_seconds_to_ns.
//
// traffic_simulator's scenario clock is
//     getCurrentScenarioTime() = seconds_since_the_simulator_started_
//                              - seconds_at_the_start_of_the_scenario_
// and the subtrahend is quiet_NaN() until SimulationClock::start() runs
// (simulation_clock.hpp:58). So EntityStatus.time is NaN for every frame published
// BEFORE the scenario clock starts -- MEASURED on a live ss2-scenario run, every sample:
//     name=ego type=0 status.time=nan msg.time=nan
//
// Upstream treats that as the definition of "not started", not as a fault:
//     if (std::isnan(evaluateSimulationTime())) { ... activateNonUserDefinedControllers(); }
//     -- openscenario_interpreter.cpp:162
// and it leaves the state only once every Autoware-controlled entity reports engaged. With
// launch_autoware:=false nothing ever engages, so it stays NaN for the whole run.
//
// WHY THIS FUNCTION EXISTS. std::llround(NaN) is unspecified; in practice it yields one
// fixed sentinel. Passing that through produced a canonical EgoState whose stamp never
// advanced while every counter in the pipeline read healthy -- received=899 published=899
// skipped=0 write_failed=0. Nothing downstream could tell that stamp from a real one. The
// epoch anchor in aw_stamp_policy.hpp is what finally caught it, by the deviation growing
// at exactly wall rate.
//
// So the contract is: a sample whose time is not finite is NOT a sample. Drop it, say why,
// and let the "no ego yet" counters carry the news -- that is honest, where a frozen
// timestamp is not.
inline bool is_usable_sim_time(double seconds) { return std::isfinite(seconds); }

// --- the mapping itself ------------------------------------------------------

// Map one SS_v2 entity onto canonical EgoState.
//
// Unit conversions: NONE.  SS_v2 is already SI with radians (geometry_msgs), which is
// what canonical EgoState wants.  That is a real property of this source, not an
// oversight -- contrast the MORAI adapter, which must convert three fields.
//
// FRAME CAVEAT -- see README "검증 항목":  `action_status.twist` is taken to be in the
// ENTITY (vehicle) frame, matching canonical `velocity`/`acceleration`.  That matches
// ROS convention for a body twist and matches how SS_v2 uses linear.x as longitudinal
// speed, but it has NOT been confirmed against a moving simulator on a curve, which is
// the only test that distinguishes body from map frame.  Until it is, treat lateral
// velocity from this adapter as unverified.
inline CanonicalEgoState map_ego_state(const Ss2Entity& ego,
                                       const std::string& run_id,
                                       std::uint64_t seq,
                                       const SynthesizedDefaults& defaults,
                                       EgoStateProvenance* provenance_out = nullptr) {
    CanonicalEgoState s;
    s.run_id = run_id;
    s.seq = seq;
    s.stamp_ns = sim_seconds_to_ns(ego.time_sec);

    s.position = ego.position;                       // m, map frame        1:1
    s.orientation_rpy = quaternion_to_rpy(ego.orientation);
    s.velocity = ego.twist_linear;                   // m/s                 1:1
    s.acceleration = ego.accel_linear;               // m/s^2               1:1
    s.yaw_rate = ego.twist_angular.z;                // rad/s               1:1

    // Not published by SS_v2 -- synthesized, and declared as such.
    s.steering_angle = defaults.steering_angle;
    s.gear = defaults.gear;

    if (provenance_out != nullptr) {
        provenance_out->steering_angle_synthesized = true;
        provenance_out->gear_synthesized = true;
    }
    return s;
}

}  // namespace sils::ss2

#endif  // SILS_ADAPTERS_SS2_EGO_MAPPING_HPP
