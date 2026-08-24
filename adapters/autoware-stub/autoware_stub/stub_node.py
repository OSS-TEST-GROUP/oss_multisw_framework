"""ROS 2 wiring for the Autoware stand-in. The decisions live in state.py.

This node exists so scenario_simulator_v2 can spawn an EgoEntity without a real Autoware.
It is NOT a simulator of Autoware: it plans nothing, perceives nothing, and publishes no
control command. Its whole job is to answer concealer's lifecycle questions so
waitForAutowareStateToBe() returns and the ego becomes drivable -- after which the actual
command comes from our vECU on /control/command/control_cmd.

Why stage 2 needs this at all: an EgoEntity IS the Autoware-driven entity in SS_v2
(Controller::isAutoware() reads `isEgo`), and only an EgoEntity brings the vehicle model +
AutowareUniverse into simple_sensor_simulator. Yet stage 2's whole point is determinism,
which a real asynchronous Autoware destroys (BR-S-3). So stage 2 needs an ego without
Autoware, and this is the smallest thing that produces one.
docs/sils/evidence/ss2-concealer-survey-2026-08-07.md §5.3.

Run:
  ros2 run autoware_stub autoware_stub
  ros2 run autoware_stub autoware_stub --ros-args -p legacy_state_topic:=true
"""

from __future__ import annotations

import rclpy
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, HistoryPolicy, QoSProfile, ReliabilityPolicy

from autoware_adapi_v1_msgs.msg import (
    LocalizationInitializationState,
    OperationModeState,
    RouteState,
)
from autoware_adapi_v1_msgs.srv import (
    ChangeOperationMode,
    ClearRoute,
    InitializeLocalization,
    SetRoute,
    SetRoutePoints,
)
from tier4_external_api_msgs.srv import Engage, SetVelocityLimit
from tier4_rtc_msgs.srv import AutoModeWithModule, CooperateCommands

from autoware_stub.state import (
    Localization,
    OperationMode,
    Route,
    StubState,
    derive,
)

# Maximally compatible QoS.
#
# Under DDS request-vs-offered rules an offered RELIABLE satisfies both RELIABLE and
# BEST_EFFORT readers, and an offered TRANSIENT_LOCAL satisfies both TRANSIENT_LOCAL and
# VOLATILE readers. Offering the strongest of each therefore matches concealer whatever it
# requests -- which matters because an RxO mismatch is SILENT: the reader simply never
# matches and neither side logs a thing. We were bitten by exactly this in stage 1.
#
# TRANSIENT_LOCAL also means a subscriber that appears late still gets the current state
# immediately instead of waiting for the next publish tick.
STATE_QOS = QoSProfile(
    depth=1,
    history=HistoryPolicy.KEEP_LAST,
    reliability=ReliabilityPolicy.RELIABLE,
    durability=DurabilityPolicy.TRANSIENT_LOCAL,
)

PUBLISH_PERIOD_S = 0.2  # concealer polls state with 1 s sleeps; 5 Hz is ample


def _const(msg_cls, name: str) -> int:
    """Read a constant off a generated message class, loudly.

    Deliberately NOT hardcoding the integers. They differ across Autoware generations, and
    a wrong integer produces a stub that publishes a valid-looking message meaning
    something else -- concealer would then wait forever for a state we believe we already
    reported. Failing at startup with the missing name is far cheaper to diagnose.
    """
    try:
        return getattr(msg_cls, name)
    except AttributeError as exc:
        raise RuntimeError(
            f"{msg_cls.__name__} has no constant {name!r}. This Autoware message "
            f"generation differs from the one concealer was built against; the stub's "
            f"state mapping needs updating. Available: "
            f"{[a for a in dir(msg_cls) if a.isupper()]}"
        ) from exc


class AutowareStub(Node):
    def __init__(self) -> None:
        super().__init__("autoware_stub")

        self.declare_parameter("legacy_state_topic", False)
        self._legacy = self.get_parameter("legacy_state_topic").value

        self._state = StubState()
        self._last_derived = None
        self._announced: set = set()

        # Resolved once, at startup, so a generation mismatch fails here rather than on
        # the first transition.
        self._loc_const = {
            Localization.UNKNOWN: _const(LocalizationInitializationState, "UNKNOWN"),
            Localization.UNINITIALIZED: _const(LocalizationInitializationState, "UNINITIALIZED"),
            Localization.INITIALIZED: _const(LocalizationInitializationState, "INITIALIZED"),
        }
        self._route_const = {
            Route.UNKNOWN: _const(RouteState, "UNKNOWN"),
            Route.UNSET: _const(RouteState, "UNSET"),
            Route.SET: _const(RouteState, "SET"),
            Route.ARRIVED: _const(RouteState, "ARRIVED"),
        }
        self._mode_const = {
            OperationMode.UNKNOWN: _const(OperationModeState, "UNKNOWN"),
            OperationMode.STOP: _const(OperationModeState, "STOP"),
            OperationMode.AUTONOMOUS: _const(OperationModeState, "AUTONOMOUS"),
        }

        self._pub_loc = self.create_publisher(
            LocalizationInitializationState, "/api/localization/initialization_state", STATE_QOS)
        self._pub_route = self.create_publisher(
            RouteState, "/api/routing/state", STATE_QOS)
        self._pub_mode = self.create_publisher(
            OperationModeState, "/api/operation_mode/state", STATE_QOS)

        self._pub_legacy = None
        if self._legacy:
            # Only imported on this path: an Autoware generation new enough to have dropped
            # autoware_system_msgs should still be able to run the adapi path.
            from autoware_system_msgs.msg import AutowareState as LegacyState
            self._legacy_cls = LegacyState
            self._pub_legacy = self.create_publisher(
                LegacyState, "/autoware/state", STATE_QOS)

        # Services. Every one flips a flag and reports success -- see _respond.
        self._services = [
            self.create_service(InitializeLocalization, "/api/localization/initialize",
                                self._on(lambda: self._state.on_initialize_localization(),
                                         "initialize_localization")),
            self.create_service(SetRoutePoints, "/api/routing/set_route_points",
                                self._on(lambda: self._state.on_set_route(), "set_route_points")),
            self.create_service(SetRoute, "/api/routing/set_route",
                                self._on(lambda: self._state.on_set_route(), "set_route")),
            self.create_service(ClearRoute, "/api/routing/clear_route",
                                self._on(lambda: self._state.on_clear_route(), "clear_route")),
            self.create_service(Engage, "/api/external/set/engage",
                                self._on(lambda: self._state.on_engage(), "engage")),
            self.create_service(ChangeOperationMode,
                                "/api/operation_mode/enable_autoware_control",
                                self._on(lambda: self._state.on_engage(),
                                         "enable_autoware_control")),
            self.create_service(ChangeOperationMode, "/api/operation_mode/change_to_stop",
                                self._on(lambda: self._state.on_change_to_stop(),
                                         "change_to_stop")),

            # --- the three that touch no state, and are NOT optional ---------------
            #
            # We first shipped without these, reasoning they were "feature" services a
            # lifecycle stub would never need. A real run said otherwise:
            #
            #   AutowareError: Service "/api/autoware/set/velocity_limit" is not ready
            #   after 180 seconds
            #
            # on RoutingAction.AcquirePositionAction -- a single-ego scenario with no RTC
            # and no NPC. concealer constructs clients for all ten and the interpreter
            # calls this one during Init, so a missing server is fatal, not degraded. The
            # only reason it took three runs to find is that concealer waits the full 180 s
            # in silence before naming it, and we had been timing out at 90.
            #
            # They flip nothing: an operation the stub does not model still has to be
            # ANSWERED, because concealer's failure mode for no answer is a hang.
            self.create_service(SetVelocityLimit, "/api/autoware/set/velocity_limit",
                                self._on(lambda: None, "set_velocity_limit")),
            self.create_service(CooperateCommands, "/api/external/set/rtc_commands",
                                self._on(lambda: None, "rtc_commands")),
            self.create_service(AutoModeWithModule, "/api/external/set/rtc_auto_mode",
                                self._on(lambda: None, "rtc_auto_mode")),
        ]

        self._timer = self.create_timer(PUBLISH_PERIOD_S, self._publish)

        self.get_logger().info(
            f"autoware_stub up. NOT a simulator of Autoware -- it answers concealer's "
            f"lifecycle so an EgoEntity can exist without one. The driving command must "
            f"come from elsewhere on /control/command/control_cmd. "
            f"state topic: {'legacy /autoware/state' if self._legacy else 'adapi (3 topics)'}")
        self._publish()

    # --- services -------------------------------------------------------------

    def _on(self, transition, label: str):
        def handler(request, response):  # noqa: ARG001 - request is intentionally ignored
            before = derive(self._state)
            transition()
            after = derive(self._state)
            first_contact = label not in self._announced
            self._announced.add(label)
            if before is not after:
                self.get_logger().info(f"{label}: {before.value} -> {after.value}")
            elif first_contact:
                # FIRST call on this service, logged even though nothing moved.
                #
                # This line is the only evidence that concealer exists at all. Its
                # constructor's final act -- before initialize(), before anything else --
                # is requestChangeToStop (field_operator_application.cpp: the task_queue
                # lambda at the end of the ctor). From our all-UNKNOWN boot state that call
                # sets operation_mode=STOP and changes NO derived state, because
                # localization is still UNKNOWN and the derivation short-circuits to
                # `initializing`.
                #
                # Without this line, an EgoEntity that was constructed and one that never
                # was produce IDENTICAL logs. We spent a run unable to tell them apart, and
                # they need opposite next actions.
                self.get_logger().info(
                    f"{label}: called by someone (no derived change; still {after.value}) "
                    f"-- first contact on this service")
            else:
                # concealer retries a request up to 30 times; a repeat is normal, not a
                # fault. Logged at debug so a retry storm is visible without being noise.
                self.get_logger().debug(f"{label}: no change (still {after.value})")
            self._respond(response, label)
            self._publish()
            return response
        return handler

    @staticmethod
    def _respond(response, label: str) -> None:
        """Fill whatever success field this response happens to have.

        The two service families disagree: autoware_adapi_v1_msgs uses
        ResponseStatus{success, message}, tier4_external_api_msgs uses
        ResponseStatus{code, message}. Setting whichever exists keeps the stub working
        across both without branching per service, and without asserting a shape we have
        not verified on this particular Autoware generation.
        """
        status = getattr(response, "status", None)
        if status is None:
            # CooperateCommands has no `status` -- concealer checks that every element of
            # `responses` succeeded, and std::all_of over the empty vector we return is
            # true. Nothing to fill.
            return
        if hasattr(status, "success"):
            status.success = True
        if hasattr(status, "code"):
            # NOT a hardcoded 0. concealer validates the tier4 family with
            #     response->status.code == tier4_external_api_msgs::msg::ResponseStatus::SUCCESS
            # (concealer/service.hpp), and nothing guarantees SUCCESS == 0 -- in this family
            # the codes are an enumeration where 0 is plausibly "unknown", not "ok". A wrong
            # constant here is invisible in our own smoke test (which never checks the code)
            # and fatal in a real run: concealer retries 30 times, then throws.
            #
            # Read it by name, same rule as _const above. Fall back to 0 only if the field
            # exists without the constant, which would be a message family we have not seen.
            status.code = getattr(type(status), "SUCCESS", 0)
        if hasattr(status, "message"):
            status.message = f"autoware_stub: {label}"

    # --- publishing -----------------------------------------------------------

    def _publish(self) -> None:
        now = self.get_clock().now().to_msg()
        s = self._state

        loc = LocalizationInitializationState()
        loc.stamp = now
        loc.state = self._loc_const[s.localization]
        self._pub_loc.publish(loc)

        route = RouteState()
        route.stamp = now
        route.state = self._route_const[s.route]
        self._pub_route.publish(route)

        mode = OperationModeState()
        mode.stamp = now
        mode.mode = self._mode_const[s.operation_mode]
        mode.is_autoware_control_enabled = s.control_enabled
        # is_autonomous_mode_available IS LOAD-BEARING. Verified in
        # concealer/legacy_autoware_state.hpp: with route SET and mode STOP,
        #     return is_autonomous_mode_available ? waiting_for_engage : planning;
        # so publishing false would pin us at `planning`, engage() would sit in
        # waitForAutowareStateToBe(planning, waiting_for_engage), and the run would die on
        # initialize_duration with no clue that one bool caused it. (A real Autoware uses
        # exactly this to say "still computing the route".)
        #
        # The rest are not read by concealer, but leaving them false would describe an
        # Autoware that can do nothing.
        mode.is_in_transition = False
        mode.is_stop_mode_available = True
        mode.is_autonomous_mode_available = True
        mode.is_local_mode_available = False
        mode.is_remote_mode_available = False
        self._pub_mode.publish(mode)

        if self._pub_legacy is not None:
            legacy = self._legacy_cls()
            legacy.stamp = now
            legacy.state = _const(self._legacy_cls, derive(s).name)
            self._pub_legacy.publish(legacy)

        current = derive(s)
        if current is not self._last_derived:
            self.get_logger().info(f"derived autoware state -> {current.value}")
            self._last_derived = current


def main(args=None) -> None:
    rclpy.init(args=args)
    node = AutowareStub()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
