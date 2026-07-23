// SDK-level PODs for the MORAI/AWSIM plant-coupling path (Option 2). DDS-free on purpose:
// behavior (vehicle_behavior.hpp) and the run loop consume these plain structs, never the
// generated DDS types (T1-A: the loop names no concrete DDS type). vehicle_wire.hpp maps
// the generated EgoState/VehicleControl <-> these. Kept in its own header so the behavior
// port compiles and unit-tests WITHOUT Fast-DDS. Units are canonical SI (see the IDLs).

#ifndef SILS_VECU_VEHICLE_SAMPLE_HPP
#define SILS_VECU_VEHICLE_SAMPLE_HPP

#include <cstdint>

namespace sils {

struct EgoStateSample {
    uint64_t seq = 0;
    int64_t stamp_ns = 0;
    double px = 0, py = 0, pz = 0;              // position, m
    double roll = 0, pitch = 0, yaw = 0;        // orientation, rad
    double vx = 0, vy = 0, vz = 0;              // velocity, m/s
    double ax = 0, ay = 0, az = 0;              // acceleration, m/s^2
    double yaw_rate = 0;                        // rad/s
    double steering_angle = 0;                  // rad
    uint8_t gear = 0;                           // 0=P 1=R 2=N 3=D
};

struct VehicleControlSample {
    uint64_t seq = 0;
    int64_t stamp_ns = 0;
    double steering_angle = 0;                  // rad
    double acceleration = 0;                    // m/s^2, signed (+ accel / - brake)
    uint8_t gear = 0;                           // 0=P 1=R 2=N 3=D
};

// Autoware control target (mirror of ControlTarget.idl) -- the "what Autoware wants" input
// to the Vehicle Interface (VCU). Same fields as VehicleControlSample, different meaning
// (target vs commanded); see docs/sils-vehicle-interface-autoware.md.
struct ControlTargetSample {
    uint64_t seq = 0;
    int64_t stamp_ns = 0;
    double steering_angle = 0;                  // rad
    double acceleration = 0;                    // m/s^2
    uint8_t gear = 0;                           // 0=P 1=R 2=N 3=D
};

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_SAMPLE_HPP
