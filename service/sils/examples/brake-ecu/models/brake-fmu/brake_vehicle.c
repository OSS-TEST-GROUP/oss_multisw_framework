/* brake_vehicle.c -- minimal FMI 2.0 Co-Simulation FMU: the brake vehicle plant.
 *
 * The same physics as the code-path BrakePlant (../../src/brake_plant.hpp), exported
 * behind the FMI 2.0 co-sim C API so it drops in via sils::MakeFmuPlant with no change
 * to the ECU's tick loop:
 *     dv/dt = -(GAIN * force) - DRAG * speed      (forward-Euler in fmi2DoStep)
 *     speed clamped at >= 0                        (no reverse motion under braking)
 *
 * GAIN/DRAG are baked in here (an FMU is a self-contained model; the FmuPlant loader
 * only drives input/output value references, not parameters). force is set via
 * fmi2SetReal(vr=1), speed read via fmi2GetReal(vr=0); initial speed is the output's
 * start value from modelDescription.xml.
 *
 * This is deliberately dependency-light: it needs only the three FMI 2.0 headers
 * (fmi2TypesPlatform.h / fmi2FunctionTypes.h / fmi2Functions.h), which build.sh fetches
 * from the Modelica FMI standard. Every exported fmi2* function is defined so the loader
 * (fmi4c) resolves them all; the co-sim-relevant ones carry logic, the rest are safe
 * no-ops. NOT a certified FMU -- a teaching-grade reference for the FMU plant path.
 */

#include <stdlib.h>
#include <string.h>

/* No FMI2_FUNCTION_PREFIX: a shared-library FMU exports the plain fmi2* names, which is
 * what the loader (fmi4c) resolves by dlsym. (The prefix is only for statically linking
 * several FMUs into one executable.) */
#include "fmi2Functions.h"

/* ---- model constants + value references ---------------------------------- */
#define VR_SPEED 0u  /* output */
#define VR_FORCE 1u  /* input  */

static const double GAIN = 0.02; /* brake force -> deceleration gain (1/mass)      */
static const double DRAG = 0.20; /* speed-proportional drag coefficient            */
static const double SPEED0 = 30.0; /* initial speed (matches modelDescription start) */

typedef struct {
    double speed;    /* state / output */
    double force;    /* staged input   */
    fmi2CallbackLogger logger;
    fmi2ComponentEnvironment env;
    char* name;
} BrakeFmu;

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
    (void)fmuType; (void)guid; (void)resourceLocation; (void)visible; (void)loggingOn;
    BrakeFmu* m = (BrakeFmu*)calloc(1, sizeof(BrakeFmu));
    if (!m) return NULL;
    m->speed = SPEED0;
    m->force = 0.0;
    if (functions) {
        m->logger = functions->logger;
        m->env = functions->componentEnvironment;
    }
    if (instanceName) {
        m->name = (char*)malloc(strlen(instanceName) + 1);
        if (m->name) strcpy(m->name, instanceName);
    }
    return (fmi2Component)m;
}

void fmi2FreeInstance(fmi2Component c) {
    BrakeFmu* m = (BrakeFmu*)c;
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

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    BrakeFmu* m = (BrakeFmu*)c;
    if (!m) return fmi2Error;
    m->speed = SPEED0;  /* initialization complete: output holds its start value */
    return fmi2OK;
}

fmi2Status fmi2Terminate(fmi2Component c) { (void)c; return fmi2OK; }

fmi2Status fmi2Reset(fmi2Component c) {
    BrakeFmu* m = (BrakeFmu*)c;
    if (!m) return fmi2Error;
    m->speed = SPEED0;
    m->force = 0.0;
    return fmi2OK;
}

/* ---- get/set --------------------------------------------------------------- */
fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                       fmi2Real value[]) {
    BrakeFmu* m = (BrakeFmu*)c;
    if (!m) return fmi2Error;
    for (size_t i = 0; i < n; ++i) {
        switch (vr[i]) {
            case VR_SPEED: value[i] = m->speed; break;
            case VR_FORCE: value[i] = m->force; break;
            default: return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t n,
                       const fmi2Real value[]) {
    BrakeFmu* m = (BrakeFmu*)c;
    if (!m) return fmi2Error;
    for (size_t i = 0; i < n; ++i) {
        switch (vr[i]) {
            case VR_FORCE: m->force = value[i]; break;
            case VR_SPEED: m->speed = value[i]; break;  /* allow init override */
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
    (void)currentCommunicationPoint; (void)noSetFMUStatePriorToCurrentPoint;
    BrakeFmu* m = (BrakeFmu*)c;
    if (!m) return fmi2Error;
    const double dt = communicationStepSize;
    m->speed += (-(GAIN * m->force) - DRAG * m->speed) * dt;
    if (m->speed < 0.0) m->speed = 0.0;
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
