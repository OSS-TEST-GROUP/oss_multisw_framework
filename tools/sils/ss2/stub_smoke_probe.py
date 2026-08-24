#!/usr/bin/env python3
"""Drive autoware_stub through concealer's sequence and check what lands on the wire.

Runs INSIDE the SS_v2 image (it needs rclpy and Autoware's message packages); the wrapper
that starts the container is tools/sils/ss2/smoke_autoware_stub.sh.

WHAT THIS PROVES, AND WHAT IT DOES NOT

The stub's unit tests cover state.py, which has no ROS in it. Everything between that state
machine and concealer is untested: whether the message packages exist in this image, whether
the constants the stub reads by name are there, whether the QoS matches, whether the services
answer. This script is exactly that gap -- it plays concealer's part by hand.

It does NOT prove concealer accepts the result. concealer only appears with an EgoEntity,
which needs a scenario asset we do not have yet (B1-4). So a PASS here means "the stub does
what we designed it to do"; it does not yet mean "stage 2 works".

WHY IT ASSERTS RAW VALUES RATHER THAN derive()

state.derive() is OUR MODEL of concealer's rule, hand-copied from legacy_autoware_state.hpp.
Asserting the stub against it would be circular -- the model would have to be wrong in the
same way twice to be caught. So the expectations below are written independently, in terms
of the constant NAMES that go out on the wire, and derive() is only reported alongside as an
informational column. If the two ever disagree, that disagreement is the finding.

Exit: 0 the full sequence was observed, 1 it was not, 2 setup/usage.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from dataclasses import dataclass, field

EXIT_OK, EXIT_FAIL, EXIT_USAGE = 0, 1, 2

# Message packages the stub needs. Checked before anything is started so a missing one is
# reported as "this image lacks X" rather than as a container that died on import.
#   (module, symbols) -- symbols are the constants the stub reads by NAME via _const().
REQUIRED_MSGS = [
    ("autoware_adapi_v1_msgs.msg", ["LocalizationInitializationState", "OperationModeState",
                                    "RouteState"]),
    ("autoware_adapi_v1_msgs.srv", ["ChangeOperationMode", "ClearRoute",
                                    "InitializeLocalization", "SetRoute", "SetRoutePoints"]),
    ("tier4_external_api_msgs.srv", ["Engage", "SetVelocityLimit"]),
    # Not optional, despite touching no state -- a real run died on a missing
    # /api/autoware/set/velocity_limit server. See stub_node.py.
    ("tier4_rtc_msgs.srv", ["AutoModeWithModule", "CooperateCommands"]),
]
REQUIRED_CONSTANTS = {
    ("autoware_adapi_v1_msgs.msg", "LocalizationInitializationState"):
        ["UNKNOWN", "UNINITIALIZED", "INITIALIZED"],
    ("autoware_adapi_v1_msgs.msg", "RouteState"): ["UNKNOWN", "UNSET", "SET", "ARRIVED"],
    ("autoware_adapi_v1_msgs.msg", "OperationModeState"): ["UNKNOWN", "STOP", "AUTONOMOUS"],
}
# Only needed for --ros-args -p legacy_state_topic:=true. Absence is reported, not fatal:
# newer Autoware generations have dropped it and the adapi path is the pinned default.
OPTIONAL_MSGS = [("autoware_system_msgs.msg", ["AutowareState"])]


def check_messages(verbose: bool = True) -> int:
    """Import-and-getattr every name the stub depends on. This is the single most likely
    failure mode of the whole exercise: the stub deliberately resolves constants by name at
    startup, so a generation mismatch shows up here rather than as a wrong integer silently
    meaning a different state."""
    missing: list[str] = []
    for mod, symbols in REQUIRED_MSGS:
        try:
            m = __import__(mod, fromlist=symbols)
        except ImportError as exc:
            missing.append(f"{mod}  (whole package: {exc})")
            continue
        for s in symbols:
            if not hasattr(m, s):
                missing.append(f"{mod}.{s}")
    for (mod, cls_name), names in REQUIRED_CONSTANTS.items():
        try:
            cls = getattr(__import__(mod, fromlist=[cls_name]), cls_name)
        except (ImportError, AttributeError):
            continue  # already reported above
        for n in names:
            if not hasattr(cls, n):
                have = [a for a in dir(cls) if a.isupper()]
                missing.append(f"{mod}.{cls_name}.{n}  (has: {', '.join(have)})")

    optional_missing = []
    for mod, symbols in OPTIONAL_MSGS:
        try:
            m = __import__(mod, fromlist=symbols)
            optional_missing += [f"{mod}.{s}" for s in symbols if not hasattr(m, s)]
        except ImportError:
            optional_missing.append(mod)

    if verbose:
        if missing:
            print("message check: FAIL")
            for x in missing:
                print(f"  missing  {x}")
            print()
            print("  This image's Autoware message generation differs from the one the stub")
            print("  was written against. Either the packages are not in this image at all")
            print("  (wrong image / workspace not sourced), or the constants were renamed.")
            print("  The stub's _const() would raise on exactly these names at startup.")
        else:
            print("message check: ok -- every package and constant the stub reads is present")
        if optional_missing:
            print(f"  note: legacy path unavailable ({', '.join(optional_missing)}) -- "
                  f"only affects -p legacy_state_topic:=true, not the adapi path")
    return EXIT_FAIL if missing else EXIT_OK


# --- the expectation ----------------------------------------------------------
#
# Written in constant NAMES on purpose: the integers differ across Autoware generations, and
# a table of integers would be asserting something we do not actually care about.

@dataclass
class Step:
    label: str
    service: tuple | None          # (name, srv_type_path) or None for the boot observation
    localization: str
    route: str
    mode: str
    control: bool
    derived: str                   # informational: what we believe concealer will read


STEPS = [
    # Boot MUST be all-UNKNOWN. If the stub came up with concrete values, `initializing`
    # would already be in the past by the time concealer subscribes, and initialize() would
    # wait out its 30 s for a transition that already happened.
    Step("boot (no calls yet)", None,
         "UNKNOWN", "UNKNOWN", "UNKNOWN", False, "initializing"),
    Step("initialize_localization",
         ("/api/localization/initialize", ("autoware_adapi_v1_msgs.srv", "InitializeLocalization")),
         "INITIALIZED", "UNSET", "STOP", False, "waiting_for_route"),
    # Route SET but control still OFF. Enabling control here would jump straight to
    # `driving` and skip `waiting_for_engage` -- the state engage() blocks on.
    Step("set_route_points",
         ("/api/routing/set_route_points", ("autoware_adapi_v1_msgs.srv", "SetRoutePoints")),
         "INITIALIZED", "SET", "STOP", False, "waiting_for_engage"),
    Step("engage",
         ("/api/external/set/engage", ("tier4_external_api_msgs.srv", "Engage")),
         "INITIALIZED", "SET", "AUTONOMOUS", True, "driving"),
]

# The four services concealer's happy path never touches -- and one repeat call.
#
# STEPS above proves the sequence works. It leaves 4 of the stub's 7 services never invoked,
# including /api/operation_mode/enable_autoware_control, which is concealer's ALTERNATIVE
# engage path -- a scenario taking that branch would hit code no test had run. The order
# below also walks BACKWARDS out of driving and returns, which is the only thing that
# exercises clear_route/change_to_stop at all.
#
# The last entry calls a service that is already satisfied. concealer retries a request up
# to 30 times, so every transition has to be idempotent; that claim was untested until here.
EXTRA_STEPS = [
    Step("change_to_stop (leave driving)",
         ("/api/operation_mode/change_to_stop",
          ("autoware_adapi_v1_msgs.srv", "ChangeOperationMode")),
         "INITIALIZED", "SET", "STOP", False, "waiting_for_engage"),
    Step("enable_autoware_control (alt engage)",
         ("/api/operation_mode/enable_autoware_control",
          ("autoware_adapi_v1_msgs.srv", "ChangeOperationMode")),
         "INITIALIZED", "SET", "AUTONOMOUS", True, "driving"),
    Step("clear_route",
         ("/api/routing/clear_route", ("autoware_adapi_v1_msgs.srv", "ClearRoute")),
         "INITIALIZED", "UNSET", "STOP", False, "waiting_for_route"),
    Step("set_route (not _points)",
         ("/api/routing/set_route", ("autoware_adapi_v1_msgs.srv", "SetRoute")),
         "INITIALIZED", "SET", "STOP", False, "waiting_for_engage"),
    Step("set_route again (idempotent)",
         ("/api/routing/set_route", ("autoware_adapi_v1_msgs.srv", "SetRoute")),
         "INITIALIZED", "SET", "STOP", False, "waiting_for_engage"),
    # The three that change nothing. Included precisely BECAUSE they change nothing: a
    # service that flips no state is exactly the kind a stub silently omits, and omitting
    # /api/autoware/set/velocity_limit cost three runs and 180 s of silence each. What is
    # being asserted here is only "it answers" -- the state columns repeat the row above.
    Step("set_velocity_limit (no-op)",
         ("/api/autoware/set/velocity_limit",
          ("tier4_external_api_msgs.srv", "SetVelocityLimit")),
         "INITIALIZED", "SET", "STOP", False, "waiting_for_engage"),
    Step("rtc_commands (no-op)",
         ("/api/external/set/rtc_commands", ("tier4_rtc_msgs.srv", "CooperateCommands")),
         "INITIALIZED", "SET", "STOP", False, "waiting_for_engage"),
    Step("rtc_auto_mode (no-op)",
         ("/api/external/set/rtc_auto_mode", ("tier4_rtc_msgs.srv", "AutoModeWithModule")),
         "INITIALIZED", "SET", "STOP", False, "waiting_for_engage"),
]


def const_names(cls) -> dict:
    """value -> [names]. A list because generations sometimes alias two names to one value;
    collapsing them to one name would produce a spurious mismatch."""
    out: dict = {}
    for n in dir(cls):
        v = getattr(cls, n)
        if n.isupper() and isinstance(v, int) and not isinstance(v, bool):
            out.setdefault(v, []).append(n)
    return out


@dataclass
class Observed:
    localization: int | None = None
    route: int | None = None
    mode: int | None = None
    control: bool | None = None
    seen: set = field(default_factory=set)


def run(args) -> int:
    import rclpy
    from rclpy.node import Node
    from rclpy.qos import DurabilityPolicy, HistoryPolicy, QoSProfile, ReliabilityPolicy

    from autoware_adapi_v1_msgs.msg import (
        LocalizationInitializationState, OperationModeState, RouteState)

    # Mirror the stub's offer exactly. Requesting TRANSIENT_LOCAL also means we get the
    # current state the moment we match, instead of waiting up to a publish period -- which
    # matters for the boot observation, where we must not miss the all-UNKNOWN window.
    qos = QoSProfile(depth=1, history=HistoryPolicy.KEEP_LAST,
                     reliability=ReliabilityPolicy.RELIABLE,
                     durability=DurabilityPolicy.TRANSIENT_LOCAL)

    loc_names = const_names(LocalizationInitializationState)
    route_names = const_names(RouteState)
    mode_names = const_names(OperationModeState)

    rclpy.init()
    node = Node("stub_smoke_probe")
    obs = Observed()

    def on_loc(m):
        obs.localization = m.state
        obs.seen.add("localization")

    def on_route(m):
        obs.route = m.state
        obs.seen.add("route")

    def on_mode(m):
        obs.mode = m.mode
        obs.control = m.is_autoware_control_enabled
        obs.seen.add("operation_mode")

    node.create_subscription(
        LocalizationInitializationState, "/api/localization/initialization_state", on_loc, qos)
    node.create_subscription(RouteState, "/api/routing/state", on_route, qos)
    node.create_subscription(OperationModeState, "/api/operation_mode/state", on_mode, qos)

    def spin(seconds: float) -> None:
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            rclpy.spin_once(node, timeout_sec=0.02)

    def matches(step: Step) -> bool:
        return (obs.localization in loc_names
                and step.localization in loc_names.get(obs.localization, [])
                and step.route in route_names.get(obs.route, [])
                and step.mode in mode_names.get(obs.mode, [])
                and obs.control == step.control)

    def wait_until(step: Step, timeout: float) -> bool:
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            rclpy.spin_once(node, timeout_sec=0.05)
            if matches(step):
                return True
        return False

    def describe() -> str:
        def n(table, v):
            if v is None:
                return "(never received)"
            return "/".join(table.get(v, [f"?{v}"]))
        return (f"localization={n(loc_names, obs.localization)} "
                f"route={n(route_names, obs.route)} "
                f"mode={n(mode_names, obs.mode)} "
                f"control={obs.control}")

    def derived_now() -> str:
        """Informational only -- see the module docstring on why this is not the assertion."""
        try:
            from autoware_stub.state import (
                Localization, OperationMode, Route, StubState, derive)
        except ImportError:
            return "(state.py not importable)"

        def pick(enum_cls, table, v, default):
            for nm in table.get(v, []):
                try:
                    return enum_cls[nm]
                except KeyError:
                    continue
            return default
        s = StubState(
            localization=pick(Localization, loc_names, obs.localization, Localization.UNKNOWN),
            route=pick(Route, route_names, obs.route, Route.UNKNOWN),
            operation_mode=pick(OperationMode, mode_names, obs.mode, OperationMode.UNKNOWN),
            control_enabled=bool(obs.control))
        return derive(s).value

    print("stub_smoke_probe: driving autoware_stub through concealer's sequence")
    print(f"  domain {args.domain}   step timeout {args.timeout}s")
    print()

    # Discovery. Judged separately from a state mismatch: "the topics never appeared" is a
    # bring-up problem (stub down, wrong domain, RxO mismatch -- which is silent on both
    # sides), not a state-machine problem, and the two want different next actions.
    end = time.monotonic() + args.discovery_timeout
    while time.monotonic() < end and len(obs.seen) < 3:
        rclpy.spin_once(node, timeout_sec=0.05)
    if len(obs.seen) < 3:
        absent = {"localization", "route", "operation_mode"} - obs.seen
        print(f"  DISCOVERY FAILED: never received {', '.join(sorted(absent))} "
              f"in {args.discovery_timeout}s")
        print()
        print("  This is not a state-machine result -- nothing was heard at all. In order:")
        print( "    1. is the stub process alive?  (its log is the wrapper's --out dir)")
        print(f"    2. same ROS_DOMAIN_ID on both sides?  probe used {args.domain}")
        print( "    3. same RMW on both sides? a request-vs-offered mismatch is SILENT --")
        print( "       neither the reader nor the writer logs anything, they just never match")
        node.destroy_node()
        rclpy.shutdown()
        return EXIT_FAIL

    W = 36  # label column; widest label sets it

    def walk(steps: list) -> tuple[bool, list]:
        """Run a list of steps, stopping at the first failure.

        Stopping matters: after a missed transition the stub's state no longer matches what
        the remaining steps assume, so every later row would be a derived failure and the
        real one would be buried in the middle of a wall of FAILs."""
        out: list = []
        for step in steps:
            if step.service is not None:
                svc_name, (mod, cls_name) = step.service
                srv_cls = getattr(__import__(mod, fromlist=[cls_name]), cls_name)
                client = node.create_client(srv_cls, svc_name)
                if not client.wait_for_service(timeout_sec=args.timeout):
                    print(f"  {step.label:<{W}} {'FAIL':<8} service {svc_name} never appeared")
                    out.append({"step": step.label, "ok": False,
                                "detail": f"service {svc_name} absent"})
                    return False, out
                # The request is empty on purpose: the stub ignores its content entirely (it
                # models lifecycle, not routing), and constructing a plausible route here
                # would imply a coupling that does not exist.
                future = client.call_async(srv_cls.Request())
                rclpy.spin_until_future_complete(node, future, timeout_sec=args.timeout)
                if not future.done():
                    print(f"  {step.label:<{W}} {'FAIL':<8} no response from {svc_name} "
                          f"in {args.timeout}s")
                    out.append({"step": step.label, "ok": False,
                                "detail": f"service {svc_name} did not respond"})
                    return False, out

            got = wait_until(step, args.timeout)
            want = (f"localization={step.localization} route={step.route} "
                    f"mode={step.mode} control={step.control}")
            print(f"  {step.label:<{W}} {'ok' if got else 'FAIL':<8} {describe()}"
                  f"{'' if got else chr(10) + ' ' * (W + 12) + 'expected  ' + want}")
            out.append({"step": step.label, "ok": got, "observed": describe(),
                        "expected": want, "derived_model": derived_now()})
            if not got:
                return False, out
        return True, out

    print(f"  {'STEP':<{W}} {'RESULT':<8} OBSERVED")
    print("  " + "-" * 112)
    ok, results = walk(STEPS)

    if ok and not args.core_only:
        # Only after the required sequence passes. Running these against a stub that already
        # failed would produce failures whose cause is the earlier step, not the service.
        print()
        print("  remaining services (not on concealer's happy path) + idempotence")
        print("  " + "-" * 112)
        extra_ok, extra = walk(EXTRA_STEPS)
        results += extra
        ok = ok and extra_ok

    print()
    for r in results:
        if r.get("derived_model"):
            print(f"  our model reads {r['step']:<{W}} as  {r['derived_model']}")

    if args.json:
        # Fail soft. This runs in a container writing to a host bind mount, so the uid may
        # simply not own the directory -- and losing the json is not a reason to discard a
        # verdict that already printed. The console output above is the primary record.
        try:
            with open(args.json, "w") as fh:
                json.dump({"domain": args.domain, "pass": ok, "steps": results}, fh, indent=2)
            print(f"\n  json  {args.json}")
        except OSError as exc:
            print(f"\n  (could not write {args.json}: {exc} -- verdict below is unaffected)")

    print()
    if ok:
        print("  VERDICT: PASS -- the stub walks initializing -> waiting_for_route ->")
        print("           waiting_for_engage -> driving, and every value on the wire is the")
        print("           one concealer's derivation needs.")
        if not args.core_only:
            print("           All 7 services answer, including the alternative engage path,")
            print("           and repeating a satisfied request changes nothing.")
        print()
        print("  Still unproven: that concealer itself accepts this. That needs an EgoEntity,")
        print("  i.e. an isEgo=true scenario asset (B1-4). This is a component test.")
    else:
        failed = next((r for r in results if not r["ok"]), None)
        print(f"  VERDICT: FAIL at '{failed['step'] if failed else '?'}'")
        print()
        print("  A stuck step means concealer would block there too -- and it would block")
        print("  silently for initialize_duration (default 30 s) before throwing")
        print("  AutowareError, with nothing in the message pointing at which step.")

    node.destroy_node()
    rclpy.shutdown()
    return EXIT_OK if ok else EXIT_FAIL


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--check-messages", action="store_true",
                   help="only verify the message packages/constants, then exit")
    p.add_argument("--domain", type=int, default=0, help="reported in output; set by env")
    p.add_argument("--timeout", type=float, default=5.0, help="per-step wait, seconds")
    p.add_argument("--discovery-timeout", type=float, default=15.0)
    p.add_argument("--core-only", action="store_true",
                   help="only concealer's happy path; skip the 4 unexercised services")
    p.add_argument("--json", default="")
    args = p.parse_args()

    if args.check_messages:
        return check_messages()
    # Quietly here: the wrapper already ran --check-messages and printed the result, so
    # repeating it makes the report look like it ran twice. On failure, re-run loudly --
    # that is when the detail is worth the duplication.
    if check_messages(verbose=False) != EXIT_OK:
        check_messages()
        print("\nrefusing to continue: the stub cannot start against these messages")
        return EXIT_FAIL
    return run(args)


if __name__ == "__main__":
    sys.exit(main())
