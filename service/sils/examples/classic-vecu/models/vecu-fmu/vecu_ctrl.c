/* vecu_ctrl.c -- a stand-in "V-ECU FMU": an AUTOSAR Classic ECU's control function exported
 * behind the FMI 2.0 Co-Simulation C API. In a real project you do NOT hand-write this: your
 * AUTOSAR toolchain + a virtualization tool (dSPACE VEOS, Synopsys Silver, Vector
 * vVIRTUALtarget, ETAS...) takes the Classic SWC + its generated RTE + BSW stubs and emits a
 * V-ECU FMU. This file is a teaching-grade approximation of that OUTPUT so the classic-vecu
 * example has something to load -- a longitudinal speed controller (the ECU's one runnable):
 *
 *     accel_cmd = KP * (V_SETPOINT - vx)        (computed in fmi2DoStep)
 *
 * The controller reads vx via fmi2SetReal(vr=0) and writes accel via fmi2GetReal(vr=1). The
 * calibrations (V_SETPOINT/KP) are baked in, as a real V-ECU bakes in its cal set. It needs
 * only the three FMI 2.0 headers build.sh fetches; every exported fmi2* symbol is defined so
 * fmi4c resolves them all. NOT a certified FMU, and NOT a real RTE/BSW virtualization -- that
 * is the toolchain's job, out of scope for libsils_vecu (we provide the FMI socket).
 */

#include <stdlib.h>
#include <string.h>

#include "fmi2Functions.h"

/* ---- value references + baked-in calibrations ---------------------------- */
#define VR_VX 0u    /* input : measured longitudinal speed, m/s */
#define VR_ACCEL 1u /* output: acceleration command, m/s^2      */

static const double V_SETPOINT = 10.0; /* target speed, m/s (a cal) */
static const double KP = 0.8;          /* longitudinal P gain (a cal) */

typedef struct {
    double vx;    /* staged input  */
    double accel; /* output        */
    char* name;
} VecuFmu;

/* ---- common ---------------------------------------------------------------- */
const char* fmi2GetTypesPlatform(void) { return fmi2TypesPlatform; }
const char* fmi2GetVersion(void) { return "2.0"; }

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean b, size_t n,
                               const fmi2String cats[]) {
    (void)c; (void)b; (void)n; (void)cats; return fmi2OK;
}

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType,
                              fmi2String guid, fmi2String resourceLocation,
                              const fmi2CallbackFunctions* functions,
                              fmi2Boolean visible, fmi2Boolean loggingOn) {
    (void)fmuType; (void)guid; (void)resourceLocation; (void)functions;
    (void)visible; (void)loggingOn;
    VecuFmu* m = (VecuFmu*)calloc(1, sizeof(VecuFmu));
    if (!m) return NULL;
    m->vx = 0.0;
    m->accel = 0.0;
    if (instanceName) {
        m->name = (char*)malloc(strlen(instanceName) + 1);
        if (m->name) strcpy(m->name, instanceName);
    }
    return (fmi2Component)m;
}

void fmi2FreeInstance(fmi2Component c) {
    VecuFmu* m = (VecuFmu*)c;
    if (!m) return;
    free(m->name);
    free(m);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined,
                               fmi2Real tolerance, fmi2Real startTime,
                               fmi2Boolean stopTimeDefined, fmi2Real stopTime) {
    (void)c; (void)toleranceDefined; (void)tolerance; (void)startTime;
    (void)stopTimeDefined; (void)stopTime; return fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) { (void)c; return fmi2OK; }
fmi2Status fmi2ExitInitializationMode(fmi2Component c) { (void)c; return fmi2OK; }
fmi2Status fmi2Terminate(fmi2Component c) { (void)c; return fmi2OK; }

fmi2Status fmi2Reset(fmi2Component c) {
    VecuFmu* m = (VecuFmu*)c;
    if (!m) return fmi2Error;
    m->vx = 0.0;
    m->accel = 0.0;
    return fmi2OK;
}

/* ---- get/set --------------------------------------------------------------- */
fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                       fmi2Real value[]) {
    VecuFmu* m = (VecuFmu*)c;
    if (!m) return fmi2Error;
    for (size_t i = 0; i < n; ++i) {
        switch (vr[i]) {
            case VR_VX: value[i] = m->vx; break;
            case VR_ACCEL: value[i] = m->accel; break;
            default: return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                       const fmi2Real value[]) {
    VecuFmu* m = (VecuFmu*)c;
    if (!m) return fmi2Error;
    for (size_t i = 0; i < n; ++i) {
        switch (vr[i]) {
            case VR_VX: m->vx = value[i]; break;
            case VR_ACCEL: m->accel = value[i]; break;
            default: return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                          fmi2Integer value[]) {
    (void)c; (void)vr; (void)n; (void)value; return fmi2OK;
}
fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                          fmi2Boolean value[]) {
    (void)c; (void)vr; (void)n; (void)value; return fmi2OK;
}
fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                         fmi2String value[]) {
    (void)c; (void)vr; (void)n; (void)value; return fmi2OK;
}
fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                          const fmi2Integer value[]) {
    (void)c; (void)vr; (void)n; (void)value; return fmi2OK;
}
fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                          const fmi2Boolean value[]) {
    (void)c; (void)vr; (void)n; (void)value; return fmi2OK;
}
fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                         const fmi2String value[]) {
    (void)c; (void)vr; (void)n; (void)value; return fmi2OK;
}

/* ---- FMU state (not supported) -------------------------------------------- */
fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* s) { (void)c; (void)s; return fmi2Error; }
fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate s) { (void)c; (void)s; return fmi2Error; }
fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* s) { (void)c; (void)s; return fmi2Error; }
fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate s, size_t* sz) {
    (void)c; (void)s; (void)sz; return fmi2Error;
}
fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate s, fmi2Byte b[], size_t sz) {
    (void)c; (void)s; (void)b; (void)sz; return fmi2Error;
}
fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte b[], size_t sz,
                                   fmi2FMUstate* s) {
    (void)c; (void)b; (void)sz; (void)s; return fmi2Error;
}

fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference u[],
                                        size_t nu, const fmi2ValueReference z[], size_t nz,
                                        const fmi2Real dv[], fmi2Real dz[]) {
    (void)c; (void)u; (void)nu; (void)z; (void)nz; (void)dv; (void)dz; return fmi2Error;
}

/* ---- co-simulation --------------------------------------------------------- */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[],
                                       size_t n, const fmi2Integer order[],
                                       const fmi2Real value[]) {
    (void)c; (void)vr; (void)n; (void)order; (void)value; return fmi2Error;
}
fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[],
                                        size_t n, const fmi2Integer order[],
                                        fmi2Real value[]) {
    (void)c; (void)vr; (void)n; (void)order; (void)value; return fmi2Error;
}

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint,
                      fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    (void)currentCommunicationPoint; (void)communicationStepSize;
    (void)noSetFMUStatePriorToCurrentPoint;
    VecuFmu* m = (VecuFmu*)c;
    if (!m) return fmi2Error;
    /* The ECU's control runnable: hold target speed. Stateless P law -- the "step" just
     * recomputes the output from the latest input (dt is irrelevant for a pure controller). */
    m->accel = KP * (V_SETPOINT - m->vx);
    return fmi2OK;
}

fmi2Status fmi2CancelStep(fmi2Component c) { (void)c; return fmi2Error; }

fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind k, fmi2Status* v) {
    (void)c; (void)k; (void)v; return fmi2Discard;
}
fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind k, fmi2Real* v) {
    (void)c; (void)k; if (v) *v = 0.0; return fmi2Discard;
}
fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind k, fmi2Integer* v) {
    (void)c; (void)k; if (v) *v = 0; return fmi2Discard;
}
fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind k, fmi2Boolean* v) {
    (void)c; (void)k; if (v) *v = fmi2False; return fmi2Discard;
}
fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind k, fmi2String* v) {
    (void)c; (void)k; if (v) *v = ""; return fmi2Discard;
}
