#!/usr/bin/env python3
"""Is the ego actually being driven? Watch its speed and say so.

Runs INSIDE the SS_v2 image (needs rclpy + traffic_simulator_msgs); the wrapper that
starts it and reports the containers around it is tools/sils/ss2/check_closed_loop.sh.

WHY THIS AND NOT THE CONTAINER LOGS

The stage-2 loop is four processes and each one has its own counters:

    simulator -> ss2_ego_bridge -> vECU -> ss2_control_bridge -> simulator

Every counter can look healthy while the loop is open. `ss2_ego_bridge` publishing 500
samples only proves the OUTBOUND half; `ss2_control_bridge` forwarding 500 commands only
proves a command was written to a topic. Neither says the vehicle model applied it -- a
wrong gear code, a *_STEER_VEL model reading our 0.0 velocity, an unrecognised gear
zeroing the acceleration, all leave both counters perfect and the car stationary.

The one observation that cannot be faked by a healthy-looking pipe is the ego's own speed
on /entity/status, which is the SIMULATOR's report, downstream of the vehicle model. If it
rises from rest, something applied an acceleration, and the only thing commanding one is
our vECU.

Exit: 0 the ego is moving under command, 1 it is not, 2 setup/usage.
"""

from __future__ import annotations

import argparse
import math
import sys
import time

EXIT_OK, EXIT_FAIL, EXIT_USAGE = 0, 1, 2

# The reference behaviour is a P-controller holding 10 m/s from rest (vehicle_behavior.hpp
# LongitudinalBehavior, SILS_CTRL_SETPOINT). Anything above noise counts as "moving"; the
# threshold is deliberately low because the question is "is it driven at all", not "does it
# track the setpoint".
MOVING_MPS = 0.5


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--seconds", type=float, default=10.0, help="observation window")
    # BOTH names, always. traffic_simulator publishes "entity/status" as a RELATIVE name
    # (entity_manager.hpp:70), so it lands wherever the publishing node's namespace puts it:
    #   cpp_mock_scenarios      scenario node, no namespace  -> /entity/status
    #   scenario_test_runner    interpreter, namespace=simulation -> /simulation/entity/status
    # Subscribing to one and guessing wrong reports "NOTHING RECEIVED" while the simulator
    # publishes perfectly -- which is what happened, and cost a full run. Listening to both
    # costs nothing and turns the difference into a printed fact.
    p.add_argument("--topic", action="append", default=None,
                   help="repeatable; default is both known names")
    p.add_argument("--ego-name", default="",
                   help="entity to watch; empty = the first EGO-typed one, else the only one")
    args = p.parse_args()

    import rclpy
    from rclpy.node import Node
    from rclpy.qos import DurabilityPolicy, HistoryPolicy, QoSProfile, ReliabilityPolicy
    from traffic_simulator_msgs.msg import EntityStatusWithTrajectoryArray

    # MEASURED (evidence/ss2-interop-probe-2026-08-06.md): the publisher offers
    # RELIABLE + VOLATILE. Requesting TRANSIENT_LOCAL here would be an RxO violation and
    # would simply never match -- silently, on both sides.
    qos = QoSProfile(depth=10, history=HistoryPolicy.KEEP_LAST,
                     reliability=ReliabilityPolicy.RELIABLE,
                     durability=DurabilityPolicy.VOLATILE)

    rclpy.init()
    node = Node("closed_loop_probe")

    samples: list[tuple[float, float]] = []   # (sim_time, vx)
    inventory: dict[str, int] = {}

    def on_status(msg) -> None:
        for d in msg.data:
            name = d.status.name or d.name
            inventory[name] = d.status.type.type
        ego = None
        for d in msg.data:
            name = d.status.name or d.name
            if args.ego_name:
                if name == args.ego_name:
                    ego = d
                    break
            elif d.status.type.type == 0:      # EntityType.EGO
                ego = d
                break
        if ego is None and not args.ego_name and len(msg.data) == 1:
            # Without Autoware an ego-role car is reported as VEHICLE(1), not EGO(0)
            # (adapters/ss2/README.md). With exactly one entity there is nothing to
            # confuse it with, so take it -- and say so in the output.
            ego = msg.data[0]
        if ego is None:
            return
        t = ego.status.time if ego.status.time != 0.0 else ego.time
        samples.append((t, ego.status.action_status.twist.linear.x))

    topics = args.topic or ["/entity/status", "/simulation/entity/status"]
    heard: dict[str, int] = {t: 0 for t in topics}
    for t in topics:
        node.create_subscription(
            EntityStatusWithTrajectoryArray, t,
            (lambda name: lambda m: (heard.__setitem__(name, heard[name] + 1), on_status(m)))(t),
            qos)

    print(f"closed_loop_probe: watching {', '.join(topics)} for {args.seconds:.0f}s")
    end = time.monotonic() + args.seconds
    while time.monotonic() < end:
        rclpy.spin_once(node, timeout_sec=0.05)

    node.destroy_node()
    rclpy.shutdown()

    print()
    for t, n in heard.items():
        print(f"  {t:<32} {n} message(s)")
    if inventory:
        print("  entities on the bus: " + ", ".join(
            f"'{n}'(type={t})" for n, t in sorted(inventory.items())))
        print("    [EntityType: 0=EGO 1=VEHICLE 2=PEDESTRIAN 3=MISC_OBJECT]")
    if not samples:
        print()
        print("  NOTHING RECEIVED on either name. This is not a result about the loop --")
        print("  the simulator's status topic was silent. Check the scenario is running and")
        print("  that ROS_DOMAIN_ID matches it.")
        return EXIT_FAIL

    vs = [v for _, v in samples]
    v_first, v_last, v_max = vs[0], vs[-1], max(vs)
    span = samples[-1][0] - samples[0][0]

    print()
    print(f"  samples      {len(samples)}   sim-time span {span:.2f}s")
    print(f"  vx first     {v_first:+.3f} m/s")
    print(f"  vx last      {v_last:+.3f} m/s")
    print(f"  vx max       {v_max:+.3f} m/s")

    # A stationary sim clock means the ego cannot move no matter what we command, and the
    # speed numbers below would be meaningless rather than merely disappointing.
    if span <= 0.0:
        print()
        print("  SIM TIME IS NOT ADVANCING. The scenario is loaded but not stepping, so no")
        print("  command could take effect. This is not a control-path result.")
        return EXIT_FAIL

    # Is it holding a speed rather than still changing? Judged over the last quarter of the
    # window so a run that is still accelerating is not mistaken for a settled one.
    tail = vs[max(0, len(vs) * 3 // 4):]
    steady = bool(tail) and (max(tail) - min(tail)) < 0.05 * max(1.0, abs(v_last))

    print()
    if v_max >= MOVING_MPS:
        print(f"  VERDICT: MOVING -- the ego reached {v_max:.2f} m/s.")
        print("  Nothing else in this configuration commands acceleration, so the return")
        print("  path is live: our vECU's command reached the vehicle model.")
        if steady:
            print()
            print(f"  ★ AND IT IS HOLDING {v_last:.2f} m/s, which says more than movement does.")
            print("  If the vECU is running the reference longitudinal P-controller")
            print("  (accel = Kp * (setpoint - vx), vehicle_behavior.hpp), a steady speed means")
            print("  the error went to zero -- which it can only do if the controller can SEE")
            print("  the speed. A broken STATE path would leave it reading vx=0 forever,")
            print("  commanding full acceleration forever, and the speed would run away.")
            print("  Holding a setpoint is therefore evidence that BOTH directions are live,")
            print("  not just the one this probe watches.")
            print(f"  Cross-check it against SILS_CTRL_SETPOINT (default 10.0).")
        else:
            print()
            print(f"  Still changing at the end of the window ({v_last:.2f} m/s). That is not a")
            print("  fault -- it may simply not have settled yet. But note that a speed which")
            print("  never settles is also what a BROKEN STATE PATH looks like: a P-controller")
            print("  reading vx=0 forever commands full acceleration forever. Watch longer, or")
            print("  check the outbound bridge's published count.")
        print()
        print("  NOT proven here: that it reaches the scenario's goal. The reference vECU")
        print("  behaviour pins steering to 0 (LongitudinalBehavior), so it drives straight.")
        return EXIT_OK

    print(f"  VERDICT: STATIONARY -- vx never exceeded {MOVING_MPS} m/s.")
    print()
    print("  Commands may still be flowing; a full pipe with a still car has four usual")
    print("  causes, cheapest first:")
    print("    1. ss2_control_bridge not running, or on a different ROS_DOMAIN_ID")
    print("    2. the vECU is not publishing -- check its `matched` count in that bridge")
    print("    3. gear: an unrecognised canonical gear is sent as NONE, and the simulator")
    print("       multiplies acceleration by sign_of(NONE)=0. Look for bad_gear>0.")
    print("    4. vehicle_model_type is a *_STEER_VEL variant, which reads")
    print("       longitudinal.velocity -- and we publish 0.0 there, i.e. 'stop'.")
    return EXIT_FAIL


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:  # noqa: BLE001 - a stack trace here helps nobody
        print(f"closed_loop_probe: {exc}", file=sys.stderr)
        sys.exit(EXIT_USAGE)
