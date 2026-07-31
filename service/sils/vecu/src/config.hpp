#ifndef SILS_VECU_CONFIG_HPP
#define SILS_VECU_CONFIG_HPP

// ProbeConfig + ConfigFromEnv() -- the SILS_* runtime environment parsed into one
// value struct (libsils_vecu SDK, docs/sils/sils-extension-model.md P1). Previously this
// lived inline at the top of viss_probe_main.cc; extracting it lets a developer's own
// ECU main() reuse the exact same env contract (ConfigFromEnv()) and hand the result
// to sils::RunProbe (run_loop.hpp) instead of re-parsing every SILS_* variable.
//
// Selection fields (behavior_name/plant_kind/scenario_kind) are still read here so the
// *default* probe main() can drive the built-in factories; a custom ECU may ignore them
// and construct its own IBehavior/IPlant directly, using only the transport/timing/QoS
// fields below.

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "plant.hpp"  // FmuConfig / IPlant / MakeFmuPlant

namespace sils {

inline std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : fallback;
}

inline int GetEnvIntOr(const char* name, int fallback) {
    const char* v = std::getenv(name);
    if (!v) return fallback;
    try {
        return std::stoi(v);
    } catch (const std::exception&) {
        std::cerr << "Invalid integer for " << name << "=\"" << v << "\", using default "
                  << fallback << std::endl;
        return fallback;
    }
}

inline double GetEnvDoubleOr(const char* name, double fallback) {
    const char* v = std::getenv(name);
    if (!v) return fallback;
    try {
        return std::stod(v);
    } catch (const std::exception&) {
        std::cerr << "Invalid number for " << name << "=\"" << v << "\", using default "
                  << fallback << std::endl;
        return fallback;
    }
}

// One run's fully-resolved configuration. Every field maps to exactly one SILS_*
// (or VISS_DDS_DOMAIN_ID) variable; see docs/sils/sils-understanding.md ch.3 for the
// descriptor -> SILS_* wiring that produces these at deploy time.
struct ProbeConfig {
    // identity / transport
    std::string run_id;                    // SILS_RUN_ID
    std::string instance_id;               // SILS_VECU_INSTANCE_ID
    std::string topic;                     // SILS_TOPIC (contract "viss-request")
    int domain_id = 0;                     // VISS_DDS_DOMAIN_ID
    bool durability_transient_local = true; // SILS_DURABILITY != "volatile"

    // timing
    int period_ms = 500;                   // SILS_VISS_PROBE_PERIOD_MS
    std::string clock_mode;                // SILS_CLOCK_MODE (wall|lockstep)

    // wire (DDS type/payload encoding)
    std::string wire_kind;                 // SILS_WIRE (viss|raw)

    // request payload (default viss-request behavior)
    std::string response_topic_prefix;     // SILS_VISS_PROBE_RESPONSE_TOPIC_PREFIX
    std::string request_action;            // SILS_VISS_PROBE_REQUEST_ACTION
    std::string request_path;              // SILS_VISS_PROBE_REQUEST_PATH

    // L1 protocol (request/response)
    bool reqresp = false;                  // SILS_PROTOCOL == "reqresp"
    std::string response_topic;            // SILS_RESPONSE_TOPIC
    int protocol_match_timeout_ms = 3000;  // SILS_PROTOCOL_MATCH_TIMEOUT_MS
    int protocol_settle_ms = 300;          // SILS_PROTOCOL_SETTLE_MS
    int response_drain_ms = 500;           // SILS_RESPONSE_DRAIN_MS

    // behavior selection (default factory)
    std::string behavior_name;             // SILS_BEHAVIOR
    double ctrl_kp = 0.8;                  // SILS_CTRL_KP
    double ctrl_setpoint = 10.0;           // SILS_CTRL_SETPOINT

    // scenario (fault injection)
    std::string scenario_kind;             // SILS_SCENARIO
    std::string scenario_spec;             // SILS_SCENARIO_FAULTS

    // L4 target attestation (self-declared)
    std::string attest_arch;               // SILS_ATTEST_ARCH
    std::string attest_gateway;            // SILS_ATTEST_GATEWAY
    std::string attest_protocol;           // SILS_ATTEST_PROTOCOL

    // plant selection (default factory)
    std::string plant_kind;                // SILS_PLANT
    double plant_drag = 0.5;               // SILS_PLANT_DRAG
    double plant_init = 0.0;               // SILS_PLANT_INIT
    std::string plant_replay_file;         // SILS_PLANT_REPLAY_FILE
    FmuConfig fmu;                         // SILS_FMU_FILE / _INPUT_VR / _OUTPUT_VR

    // input staging / replay (§7.3/§7.4)
    bool stage_input = false;              // SILS_STAGE_INPUT == "1"
    std::string input_topic;               // SILS_INPUT_TOPIC
    std::string late_sample_policy;        // SILS_LATE_SAMPLE_POLICY
    std::string replay_file;               // SILS_REPLAY_FILE

    // MORAI/AWSIM plant coupling (Option 2 bus-coupled world; channel_map.hpp). Empty
    // = inactive (single-endpoint behavior unchanged). Points at a channel-map YAML
    // (config/sils/channelmap/*.yaml) listing the VSS-path MIMO signals this vECU
    // subscribes to (STATE in) / publishes (ACTUATION out). See
    // docs/sils/sils-morai-plant-adapter-design.md.
    std::string channelmap_file;           // SILS_CHANNELMAP

    const char* durability_name() const {
        return durability_transient_local ? "TRANSIENT_LOCAL" : "VOLATILE";
    }
};

inline ProbeConfig ConfigFromEnv() {
    ProbeConfig c;
    c.run_id = GetEnvOr("SILS_RUN_ID", "local");
    c.instance_id = GetEnvOr("SILS_VECU_INSTANCE_ID", "viss-probe-1");
    c.topic = GetEnvOr("SILS_TOPIC", "/VIN/Vehicle");
    c.domain_id = GetEnvIntOr("VISS_DDS_DOMAIN_ID", 0);
    c.durability_transient_local = GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile";

    c.period_ms = GetEnvIntOr("SILS_VISS_PROBE_PERIOD_MS", 500);
    c.clock_mode = GetEnvOr("SILS_CLOCK_MODE", "wall");
    c.wire_kind = GetEnvOr("SILS_WIRE", "viss");

    c.response_topic_prefix =
        GetEnvOr("SILS_VISS_PROBE_RESPONSE_TOPIC_PREFIX", "sils/viss_probe/response");
    c.request_action = GetEnvOr("SILS_VISS_PROBE_REQUEST_ACTION", "get");
    c.request_path = GetEnvOr("SILS_VISS_PROBE_REQUEST_PATH", "Vehicle.Speed");

    c.reqresp = GetEnvOr("SILS_PROTOCOL", "oneway") == "reqresp";
    c.response_topic = GetEnvOr("SILS_RESPONSE_TOPIC", "sils/viss/response");
    c.protocol_match_timeout_ms = GetEnvIntOr("SILS_PROTOCOL_MATCH_TIMEOUT_MS", 3000);
    c.protocol_settle_ms = GetEnvIntOr("SILS_PROTOCOL_SETTLE_MS", 300);
    c.response_drain_ms = GetEnvIntOr("SILS_RESPONSE_DRAIN_MS", 500);

    c.behavior_name = GetEnvOr("SILS_BEHAVIOR", "viss-request");
    c.ctrl_kp = GetEnvDoubleOr("SILS_CTRL_KP", 0.8);
    c.ctrl_setpoint = GetEnvDoubleOr("SILS_CTRL_SETPOINT", 10.0);

    c.scenario_kind = GetEnvOr("SILS_SCENARIO", "nominal");
    c.scenario_spec = GetEnvOr("SILS_SCENARIO_FAULTS", "");

    c.attest_arch = GetEnvOr("SILS_ATTEST_ARCH", "unknown");
    c.attest_gateway = GetEnvOr("SILS_ATTEST_GATEWAY", "sim");
    c.attest_protocol = GetEnvOr("SILS_ATTEST_PROTOCOL", "sim");

    c.plant_kind = GetEnvOr("SILS_PLANT", "none");
    c.plant_drag = GetEnvDoubleOr("SILS_PLANT_DRAG", 0.5);
    c.plant_init = GetEnvDoubleOr("SILS_PLANT_INIT", 0.0);
    c.plant_replay_file = GetEnvOr("SILS_PLANT_REPLAY_FILE", "");
    c.fmu.file = GetEnvOr("SILS_FMU_FILE", "");
    c.fmu.input_vr = GetEnvIntOr("SILS_FMU_INPUT_VR", -1);
    c.fmu.output_vr = GetEnvIntOr("SILS_FMU_OUTPUT_VR", 0);

    c.stage_input = GetEnvOr("SILS_STAGE_INPUT", "0") == "1";
    c.input_topic = GetEnvOr("SILS_INPUT_TOPIC", "sils/input/stream");
    c.late_sample_policy = GetEnvOr("SILS_LATE_SAMPLE_POLICY", "next_tick");
    c.replay_file = GetEnvOr("SILS_REPLAY_FILE", "");

    c.channelmap_file = GetEnvOr("SILS_CHANNELMAP", "");
    return c;
}

// Convenience: build the FMU plant straight from a ProbeConfig (its .fmu fields come
// from SILS_FMU_FILE / _INPUT_VR / _OUTPUT_VR). Lets an FMU-path ECU write
// `auto plant = sils::MakeFmuPlant(cfg, err)` -- the one-line counterpart to the code
// path's `std::make_unique<MyPlant>(...)`. Delegates to the FmuConfig overload in
// plant.hpp; returns nullptr on error / when built without -DSILS_ENABLE_FMU.
inline std::unique_ptr<IPlant> MakeFmuPlant(const ProbeConfig& cfg, std::string& error) {
    return MakeFmuPlant(cfg.fmu, error);
}

}  // namespace sils

#endif  // SILS_VECU_CONFIG_HPP
