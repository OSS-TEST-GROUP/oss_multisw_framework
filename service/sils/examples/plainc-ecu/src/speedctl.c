/* speedctl.c -- the control model body (auto-code stand-in). A controls engineer would
 * generate this from a Simulink/TargetLink model; here it is hand-written to be readable,
 * but the SHAPE is what matters -- a plain-C _step() that is a pure function of (u, p).
 * A combined longitudinal + lateral proportional controller. No SDK/DDS/FMI includes. */
#include "speedctl.h"

void speedctl_initialize(speedctl_Params *p) {
    p->v_setpoint = 10.0;   /* m/s */
    p->kp_speed = 0.8;
    p->steer_setpoint = 0.1; /* rad */
    p->kp_steer = 0.5;
}

void speedctl_step(const speedctl_ExtU *u, const speedctl_Params *p, speedctl_ExtY *y) {
    /* Longitudinal: hold target speed.  accel = Kp * (v_set - v) */
    y->accel_cmd = p->kp_speed * (p->v_setpoint - u->vx);
    /* Lateral: hold target road-wheel angle.  steer = Kp * (set - angle) */
    y->steer_cmd = p->kp_steer * (p->steer_setpoint - u->steering_angle);
}
