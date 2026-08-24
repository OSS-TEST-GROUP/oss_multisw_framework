"""Pure state machine for the Autoware stand-in. No ROS imports -- testable anywhere.

WHAT THIS MODELS

scenario_simulator_v2's concealer drives an ego through initialize() -> plan() -> engage(),
and WAITS at each step for Autoware to report a matching state. Without Autoware nothing
ever reports, waitForAutowareStateToBe() spins until initialize_duration elapses, and the
run dies with AutowareError. See docs/sils/evidence/ss2-concealer-survey-2026-08-07.md §5.3.

The stub's job is to make those waits return. NOT to imitate Autoware -- it plans nothing,
perceives nothing, and issues no control. The actual driving command comes from our vECU
(docs/sils/sils-simulator-loops.md §4).

THE INDIRECTION THAT MATTERS

At architecture_type >= "awf/universe/20230906" -- the pinned default is 20240605 --
concealer does NOT read a state topic. It DERIVES the state from three ADAPI values:
localization + routing + operation_mode. So the stub does not publish "waiting_for_route";
it publishes three values whose combination concealer reads as waiting_for_route.

`derive()` below is OUR MODEL of concealer's rule, kept in sync by hand. It is not used to
produce anything on the wire -- only to log where we believe concealer thinks we are, and
to let the tests assert the required sequence. If the real run disagrees with our log, that
divergence is the finding.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class Localization(Enum):
    UNKNOWN = "unknown"
    UNINITIALIZED = "uninitialized"
    INITIALIZED = "initialized"


class Route(Enum):
    UNKNOWN = "unknown"
    UNSET = "unset"
    SET = "set"
    ARRIVED = "arrived"


class OperationMode(Enum):
    UNKNOWN = "unknown"
    STOP = "stop"
    AUTONOMOUS = "autonomous"


class AutowareState(Enum):
    """concealer::LegacyAutowareState. Only the values our sequence passes through are
    exercised; the rest exist so an unexpected combination has a name."""
    UNDEFINED = "undefined"
    INITIALIZING = "initializing"
    WAITING_FOR_ROUTE = "waiting_for_route"
    PLANNING = "planning"
    WAITING_FOR_ENGAGE = "waiting_for_engage"
    DRIVING = "driving"
    ARRIVED_GOAL = "arrived_goal"
    FINALIZING = "finalizing"


@dataclass
class StubState:
    """The three ADAPI values the stub publishes, plus the control-enabled flag.

    Starts at UNKNOWN rather than UNINITIALIZED on purpose: concealer maps any UNKNOWN to
    `initializing`, which is the state initialize() expects to see FIRST. Starting at
    UNINITIALIZED would already read as waiting_for_route and the first transition
    initialize() waits for would never be observed -- it would have happened before
    concealer subscribed.
    """

    localization: Localization = Localization.UNKNOWN
    route: Route = Route.UNKNOWN
    operation_mode: OperationMode = OperationMode.UNKNOWN
    control_enabled: bool = False

    # --- transitions, one per service concealer calls ------------------------

    def on_initialize_localization(self) -> None:
        """/api/localization/initialize -> localization becomes INITIALIZED.

        The other two leave UNKNOWN here too: concealer needs to see `initializing` before
        `waiting_for_route`, and it cannot see the former if route/mode are already
        concrete at startup. Once localization is up, all three must be concrete or the
        UNKNOWN rule pins us at `initializing` forever.
        """
        self.localization = Localization.INITIALIZED
        self.route = Route.UNSET
        self.operation_mode = OperationMode.STOP
        self.control_enabled = False

    def on_set_route(self) -> None:
        """/api/routing/set_route{,_points} -> route becomes SET.

        Control stays disabled: plan() waits for `waiting_for_engage`, and enabling control
        here would jump straight to `driving`, skipping the state engage() is waiting for.
        A skipped state is not observed, and concealer would wait for it forever.
        """
        self.route = Route.SET

    def on_clear_route(self) -> None:
        """/api/routing/clear_route -> back to waiting_for_route."""
        self.route = Route.UNSET
        self.operation_mode = OperationMode.STOP
        self.control_enabled = False

    def on_engage(self) -> None:
        """/api/external/set/engage or /api/operation_mode/enable_autoware_control.

        This is the last gate. After it concealer stops waiting and the ego is driven by
        whatever publishes /control/command/control_cmd -- our vECU.
        """
        self.operation_mode = OperationMode.AUTONOMOUS
        self.control_enabled = True

    def on_change_to_stop(self) -> None:
        """/api/operation_mode/change_to_stop."""
        self.operation_mode = OperationMode.STOP
        self.control_enabled = False


def derive(s: StubState) -> AutowareState:
    """Our model of concealer::LegacyAutowareState's ADAPI derivation.

    Mirrors external/concealer/include/concealer/legacy_autoware_state.hpp at pin
    3370c817. Hand-maintained: if that file changes, this goes stale silently, which is
    why nothing on the wire depends on it.
    """
    if (s.localization is Localization.UNKNOWN
            or s.route is Route.UNKNOWN
            or s.operation_mode is OperationMode.UNKNOWN):
        return AutowareState.INITIALIZING

    if s.localization is not Localization.INITIALIZED:
        return AutowareState.INITIALIZING

    if s.route is Route.UNSET:
        return AutowareState.WAITING_FOR_ROUTE
    if s.route is Route.ARRIVED:
        return AutowareState.ARRIVED_GOAL

    # route is SET: driving only once Autoware control is actually engaged.
    if s.operation_mode is OperationMode.AUTONOMOUS and s.control_enabled:
        return AutowareState.DRIVING
    return AutowareState.WAITING_FOR_ENGAGE


# The sequence concealer's initialize() -> plan() -> engage() waits through. Used by the
# tests to prove no step is skipped -- a skipped state is never observed, and concealer
# waits for it until initialize_duration expires.
REQUIRED_SEQUENCE = (
    AutowareState.INITIALIZING,
    AutowareState.WAITING_FOR_ROUTE,
    AutowareState.WAITING_FOR_ENGAGE,
    AutowareState.DRIVING,
)
