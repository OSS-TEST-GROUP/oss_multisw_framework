// SiLS vECU -- default VISS /VIN/Vehicle probe.
//
// This is now a thin consumer of the libsils_vecu SDK (docs/sils-extension-model.md):
// it parses the SILS_* environment (ConfigFromEnv), builds the two injected ports from
// the built-in factories per the env selection (SILS_BEHAVIOR -> IBehavior,
// SILS_PLANT -> IPlant), and hands them to sils::RunProbe, which owns the DDS setup,
// the clock -> behavior -> scenario -> publish -> plant loop, the trace contract, and
// teardown (run_loop.hpp). The clock and scenario ports are built inside RunProbe from
// the same config.
//
// A developer's own ECU main() looks exactly like this, except it constructs its own
// IBehavior/IPlant directly instead of going through the factories:
//     auto cfg = sils::ConfigFromEnv();
//     return sils::RunProbe(cfg, std::make_unique<MyBehavior>(...),
//                                std::make_unique<MyPlant>(...));

#include <iostream>
#include <memory>

#include "behavior.hpp"
#include "config.hpp"
#include "plant.hpp"
#include "run_loop.hpp"

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    // Behavior port (§11 L2): SILS_BEHAVIOR selects the built-in ("viss-request" wire
    // payload, "accumulate", "p-controller"); a custom ECU replaces this line.
    std::unique_ptr<sils::IBehavior> behavior = sils::MakeBehavior(
        cfg.behavior_name, cfg.request_action, cfg.request_path, cfg.ctrl_kp, cfg.ctrl_setpoint);

    // Plant port (§11 L3): SILS_PLANT selects "first-order"/"replay"/"fmu"; "none"
    // (default) or unknown -> nullptr. The factory owns the unknown-kind error report
    // here, since RunProbe accepts whatever port it is given (null = open loop).
    std::string plant_err;
    std::unique_ptr<sils::IPlant> plant =
        sils::MakePlant(cfg.plant_kind, cfg.plant_drag, cfg.plant_replay_file, cfg.fmu, plant_err);
    if (cfg.plant_kind != "none" && !plant) {
        std::cerr << "probe: "
                  << (plant_err.empty() ? "unknown plant: " + cfg.plant_kind : plant_err)
                  << std::endl;
        return 2;
    }

    return sils::RunProbe(cfg, std::move(behavior), std::move(plant));
}
