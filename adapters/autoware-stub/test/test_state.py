"""Tests for the Autoware stub's state machine. No ROS needed.

The thing being protected here is the SEQUENCE, not any single value. concealer waits for
each transition in turn, so a state that is skipped is a state that is never observed, and
concealer waits for it until initialize_duration expires -- a 30-second hang with no error
pointing at the cause. Every test below exists to catch a skip.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from autoware_stub.state import (  # noqa: E402
    REQUIRED_SEQUENCE,
    AutowareState,
    Localization,
    OperationMode,
    Route,
    StubState,
    derive,
)


def drive_through() -> list[AutowareState]:
    """The call order concealer uses: initialize() -> plan() -> engage()."""
    s = StubState()
    seen = [derive(s)]
    for step in (s.on_initialize_localization, s.on_set_route, s.on_engage):
        step()
        seen.append(derive(s))
    return seen


def test_full_sequence_matches_what_concealer_waits_for():
    assert drive_through() == list(REQUIRED_SEQUENCE)


def test_starts_at_initializing_not_waiting_for_route():
    """If the stub came up already concrete, `initializing` would never be OBSERVED --
    it would have passed before concealer subscribed, and initialize() would hang."""
    assert derive(StubState()) is AutowareState.INITIALIZING


def test_no_state_is_skipped():
    seen = drive_through()
    assert len(seen) == len(set(seen)), f"a state repeated, so one was skipped: {seen}"
    assert len(seen) == 4


def test_setting_a_route_does_not_jump_to_driving():
    """The regression this file is really for. Enabling control at set_route would go
    straight to DRIVING, skipping WAITING_FOR_ENGAGE -- which is exactly the state
    engage() blocks on."""
    s = StubState()
    s.on_initialize_localization()
    s.on_set_route()
    assert derive(s) is AutowareState.WAITING_FOR_ENGAGE
    assert s.control_enabled is False


def test_initialize_makes_all_three_values_concrete():
    """One value left UNKNOWN pins the derivation at `initializing` forever, because the
    UNKNOWN rule short-circuits before anything else is considered."""
    s = StubState()
    s.on_initialize_localization()
    assert s.localization is not Localization.UNKNOWN
    assert s.route is not Route.UNKNOWN
    assert s.operation_mode is not OperationMode.UNKNOWN
    assert derive(s) is AutowareState.WAITING_FOR_ROUTE


def test_any_unknown_pins_at_initializing():
    for field, value in (("localization", Localization.UNKNOWN),
                         ("route", Route.UNKNOWN),
                         ("operation_mode", OperationMode.UNKNOWN)):
        s = StubState()
        s.on_initialize_localization()
        s.on_set_route()
        s.on_engage()
        assert derive(s) is AutowareState.DRIVING
        setattr(s, field, value)
        assert derive(s) is AutowareState.INITIALIZING, f"{field}=UNKNOWN was not honoured"


def test_engage_requires_both_mode_and_control_flag():
    """AUTONOMOUS alone is not driving -- concealer checks is_autoware_control_enabled
    too. Setting only the mode would leave it waiting at WAITING_FOR_ENGAGE."""
    s = StubState()
    s.on_initialize_localization()
    s.on_set_route()
    s.operation_mode = OperationMode.AUTONOMOUS
    assert derive(s) is AutowareState.WAITING_FOR_ENGAGE
    s.control_enabled = True
    assert derive(s) is AutowareState.DRIVING


def test_clear_route_goes_back_to_waiting_for_route():
    s = StubState()
    s.on_initialize_localization()
    s.on_set_route()
    s.on_engage()
    s.on_clear_route()
    assert derive(s) is AutowareState.WAITING_FOR_ROUTE
    assert s.control_enabled is False, "control must drop with the route, or a stale " \
                                       "engage would read as driving with no route"


def test_change_to_stop_leaves_driving():
    s = StubState()
    s.on_initialize_localization()
    s.on_set_route()
    s.on_engage()
    s.on_change_to_stop()
    assert derive(s) is AutowareState.WAITING_FOR_ENGAGE


def test_arrived_reports_arrived_goal():
    s = StubState()
    s.on_initialize_localization()
    s.on_set_route()
    s.on_engage()
    s.route = Route.ARRIVED
    assert derive(s) is AutowareState.ARRIVED_GOAL


def test_engage_before_route_does_not_report_driving():
    """A scenario that engages out of order must not read as driving -- there is no route
    to drive along, and reporting DRIVING would hide the real problem."""
    s = StubState()
    s.on_initialize_localization()
    s.on_engage()
    assert derive(s) is AutowareState.WAITING_FOR_ROUTE


def test_repeated_calls_are_idempotent():
    """concealer retries service requests up to 30 times; a retry must not move us on."""
    s = StubState()
    for _ in range(3):
        s.on_initialize_localization()
    assert derive(s) is AutowareState.WAITING_FOR_ROUTE
    for _ in range(3):
        s.on_set_route()
    assert derive(s) is AutowareState.WAITING_FOR_ENGAGE
    for _ in range(3):
        s.on_engage()
    assert derive(s) is AutowareState.DRIVING
