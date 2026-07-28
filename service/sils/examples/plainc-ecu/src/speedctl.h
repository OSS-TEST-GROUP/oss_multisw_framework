/* speedctl.h -- generated-style control-model interface, as produced by Simulink
 * Embedded Coder / dSPACE TargetLink auto-coding. PLAIN C, and deliberately ignorant of
 * libsils_vecu, DDS, and FMI: this is exactly the file a controls engineer drops in from
 * their model-based-design toolchain. The SDK adapter (plainc_behavior.hpp) is the only
 * glue; this header/source pair is never edited to run in SiLS.
 *
 * Shape mirrors Embedded Coder's real-time model: ExtU (root Inports), ExtY (root
 * Outports), a params/cal struct, an _initialize() and a _step(). Swap this file for your
 * own generated model and only the field names in the adapter change.
 */
#ifndef SPEEDCTL_H
#define SPEEDCTL_H

#ifdef __cplusplus
extern "C" {
#endif

/* External inputs (root Inport signals), SI units. */
typedef struct {
    double vx;             /* measured longitudinal speed, m/s */
    double steering_angle; /* measured road-wheel angle, rad */
} speedctl_ExtU;

/* External outputs (root Outport signals), SI units. */
typedef struct {
    double accel_cmd; /* longitudinal acceleration command, m/s^2 (+ accel / - brake) */
    double steer_cmd; /* road-wheel angle command, rad */
} speedctl_ExtY;

/* Tunable parameters (Simulink workspace variables / TargetLink calibrations). */
typedef struct {
    double v_setpoint;     /* target speed, m/s */
    double kp_speed;       /* longitudinal P gain */
    double steer_setpoint; /* target road-wheel angle, rad */
    double kp_steer;       /* lateral P gain */
} speedctl_Params;

/* Set default calibrations (model init). */
void speedctl_initialize(speedctl_Params *p);

/* One control step: read u + p, write y. Pure function of its arguments (no I/O). */
void speedctl_step(const speedctl_ExtU *u, const speedctl_Params *p, speedctl_ExtY *y);

#ifdef __cplusplus
}
#endif

#endif /* SPEEDCTL_H */
