// brake-ecu (FMU path) -- the SAME brake ECU as main.cc, with the plant supplied by an
// exported .fmu instead of the hand-written BrakePlant. This file exists to make the
// extension-model claim "only the plant line changes" literal: diff it against main.cc
// and the ONLY functional difference is how `plant` is built --
//
//     code path (main.cc):     auto plant = std::make_unique<BrakePlant>(gain, drag);
//     FMU  path (main_fmu.cc):  auto plant = sils::MakeFmuPlant(cfg, err);
//
// Behavior, config, RunProbe, trace, container -- all identical. The .fmu file + its
// value references are supplied at deploy time via SILS_FMU_FILE / SILS_FMU_INPUT_VR /
// SILS_FMU_OUTPUT_VR (see models/brake-fmu/README.md). Built only when the SDK is
// compiled with -DSILS_ENABLE_FMU (fmi4c); otherwise MakeFmuPlant reports that clearly.

#include <iostream>
#include <memory>

#include "brake_behavior.hpp"
#include "config.hpp"     // sils::ConfigFromEnv / MakeFmuPlant / GetEnvDoubleOr
#include "run_loop.hpp"   // sils::RunProbe

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    const double max_force = sils::GetEnvDoubleOr("BRAKE_MAX_FORCE", 100.0);
    auto behavior = std::make_unique<BrakeBehavior>(cfg.ctrl_kp, cfg.ctrl_setpoint, max_force);

    std::string err;
    auto plant = sils::MakeFmuPlant(cfg, err);   // ── FMU path (vs code-path BrakePlant)
    if (!plant) {
        std::cerr << "brake_ecu_fmu: " << err << std::endl;
        return 2;
    }

    return sils::RunProbe(cfg, std::move(behavior), std::move(plant));
}
