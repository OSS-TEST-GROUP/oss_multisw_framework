// classic-vecu -- Case 3: an AUTOSAR Classic ECU plugged into libsils_vecu as a V-ECU FMU.
//
// A Classic ECU's application cannot be compiled standalone -- it depends on a generated RTE
// and a BSW stack. The industry-standard route into SiL is to VIRTUALIZE it as a V-ECU and
// export that as an FMU (FMI 2.0/3.0 co-sim), which every AUTOSAR toolchain + virtualization
// tool (dSPACE VEOS, Synopsys Silver, Vector vVIRTUALtarget, ETAS) can do. This example loads
// such an FMU behind the SDK's IVehicleBehavior via sils::MakeFmuBehavior -- NO C++ control
// code is written. See models/vecu-fmu/ for a stand-in V-ECU FMU to run against.
//
// We do NOT build the V-ECU (RTE/BSW virtualization is the toolchain's job); we provide the
// FMI socket + the canonical channel binding. The value-reference mapping (which FMU
// variable is which canonical signal) is supplied via env, mirroring the FMU plant path.

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "config.hpp"                // sils::ConfigFromEnv / GetEnvOr / GetEnvIntOr
#include "vehicle_fmu_behavior.hpp"  // sils::FmuBehaviorConfig / MakeFmuBehavior
#include "vehicle_run.hpp"           // sils::RunVehicleProbe

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    // Which FMU value references carry which canonical signals (per the V-ECU's
    // modelDescription.xml). Defaults match models/vecu-fmu (vx=0 in, accel=1 out).
    sils::FmuBehaviorConfig fb;
    fb.file = sils::GetEnvOr("SILS_FMU_FILE", "");
    fb.vx_vr = sils::GetEnvIntOr("SILS_FMU_VR_VX", 0);
    fb.steering_in_vr = sils::GetEnvIntOr("SILS_FMU_VR_STEER_IN", -1);
    fb.accel_out_vr = sils::GetEnvIntOr("SILS_FMU_VR_ACCEL_OUT", 1);
    fb.steer_out_vr = sils::GetEnvIntOr("SILS_FMU_VR_STEER_OUT", -1);
    fb.gear_out_vr = sils::GetEnvIntOr("SILS_FMU_VR_GEAR_OUT", -1);

    std::string err;
    auto behavior = sils::MakeFmuBehavior(fb, err);
    if (!behavior) {
        std::cerr << "classic_vecu: " << err << std::endl;
        return 2;
    }
    return sils::RunVehicleProbe(cfg, std::move(behavior));
}
