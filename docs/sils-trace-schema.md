# SiLS vECU Trace Schema — v1

> **The trace a run emits IS the public conformance boundary.** The conformance kit
> (`evaluate.py` / `parity.py` / `validate_trace.py`) and any reviewer consume only this
> JSONL stream — not the vECU's internals. So the event set + fields below, not the C++
> API, are what must stay stable across versions (see
> [sils-extension-model.md §2](sils-extension-model.md)).
>
> Machine-readable source of truth: [`config/sils/trace-schema/v1.json`](../config/sils/trace-schema/v1.json).
> This document is its human companion.

## Format

A trace is **JSON Lines**: one JSON object per line, on stdout. Other stdout (DDS logs,
coverage `profiling:` notes) may be interleaved — consumers skip non-JSON lines, so a raw
capture is valid input. Every event object carries at least the **base fields**:

| field | meaning |
|---|---|
| `run_id` | identifies one Coordinator run (`SILS_RUN_ID`). Not the vECU. |
| `event` | the event name (one of the tables below). |

Most vECU events additionally carry the **standard envelope** `instance_id`, `seq`,
`wall_time_ms`, `topic` (emitted via the `TraceEvent` helper). A few high-rate per-tick
events use a **compact** form (base + their own fields, no `seq`/`wall_time_ms`/`topic`):
`behavior_output`, `input_received`, `tick_input_frozen`, `tick_complete`. Auxiliary
producers (clock master, responder) omit `instance_id`. The schema encodes the exact
`required` set per event; unknown extra fields are always allowed (forward-compatible).

## Producers

| producer | source | role |
|---|---|---|
| `vecu` | `sils::RunProbe` (`run_loop.hpp`) | the vECU itself — **what a developer ECU emits for free** |
| `coordinator` | `run_coordinator/run_phase1.sh` | run lifecycle + terminal verdict |
| `clock_master` | `sils_clock_master.cc` | lockstep tick control plane |
| `responder` | `viss_responder_main.cc` | L1 SUT stand-in |
| `verifier` | `viss_verify_main.cc` | run verifier |
| `input_source` | `sils_input_source.cc` | async input fixture |

## vECU events (the core contract)

Required fields beyond the base `run_id`. All also carry the standard envelope
(`instance_id`/`seq`/`wall_time_ms`/`topic`) unless marked *compact*.

### Lifecycle
| event | required (beyond base + envelope) | when |
|---|---|---|
| `run_start` | `domain_id` (envelope `seq`/`topic` vECU-only) | participant created — **also emitted by the coordinator** (adds `backend`, no `seq`/`topic`); required is the intersection `instance_id`/`wall_time_ms`/`domain_id` |
| `wire_start` | `name`, `type` | wire codec registered (§P4) |
| `qos_negotiated` | `role`, `reliability`, `durability` | writer/reader QoS set |
| `behavior_start` | `name` | behavior port selected |
| `scenario_start` | `name`, `scheduled_faults` | scenario port selected |
| `target_attestation` | `arch`, `gateway`, `protocol` | L4 self-declaration |
| `plant_reset` | `model`, `mode`, `initial` (`fmi_version?`) | plant initialized |
| `ready` | — | setup complete, loop about to start |
| `run_stopping` / `run_stopped` | — | teardown |

### Per tick
| event | required | form |
|---|---|---|
| `behavior_output` | `behavior`, `tick_seq`, `value` | *compact* |
| `publish_intent` | `matched_count` | envelope |
| `request_sent` | `request_id`, `path`, `action` | envelope (L1 reqresp) |
| `scenario_fault` | `fault`, `request_id` | envelope (negative path) |
| `publish_result` | — | envelope |
| `plant_step` | `tick_seq`, `sim_time_ns`, `dt_ns`, `feedback_in`, `command`, `output` | envelope (L3) |

### Matching / L1 / staging
| event | required | notes |
|---|---|---|
| `endpoint_matched` | `matched_count` (`role?`) | pub/sub match |
| `endpoint_unmatched` | `matched_count` | unmatch |
| `protocol_on` | `mode`, `response_topic` | reqresp enabled |
| `protocol_ready` | `response_matched`, `waited_ms` | reqresp settled |
| `response_received` | `request_id`, `status` (`error_code?`) | L1 reply |
| `input_staging_on` | `input_topic` | staging enabled |
| `input_received` | `recv_seq` (`topic?`/`source?`) | *compact*; staged/replayed input |
| `tick_input_frozen` | `tick_seq`, `recv_seqs`, `count` (`source?`) | *compact*; §7.3 freeze |
| `replay_start` | `file`, `inputs` | §7.4 replay |
| `tick_complete` | `tick_seq`, `sim_time_ns` | *compact*; lockstep TickComplete |

## Auxiliary events

| event | producer | required |
|---|---|---|
| `clock_start` | clock_master | `mode`, `wall_time_ms` (`dt_ns`/`participants`/`ticks`) |
| `tick_grant` | clock_master | `tick_seq`, `sim_time_ns`, `dt_ns`, `wall_time_ms` |
| `tick_barrier` | clock_master | `tick_seq`, `completed`, `wall_time_ms` |
| `clock_stop` | clock_master | `exit_code`, `wall_time_ms` |
| `responder_start` / `responder_reply` / `responder_stop` | responder | `role` + `domain_id`/`response_topic` · `request_id`/`status` · `handled` |
| `verify_start` / `sample_received` / `verify_pass` / `verify_fail` | verifier | `expected_instance_id`, `topic`, … (`verify_pass`/`verify_fail` share `received_count`/`valid_count`/`min_samples`) |
| `input_source_start` / `input_source_sent` / `input_source_done` | input_source | `count`/`rate_ms`/`topic` · `seq`/`topic` · `sent` |

## Coordinator events (run lifecycle + terminal verdict)

The Run Coordinator (`run_phase1.sh`) merges these into the run trace; `evaluate.py`
derives the §5.2 terminal verdict from them. All carry base + `wall_time_ms`.

| event | required (beyond base + `wall_time_ms`) | role |
|---|---|---|
| `domain_leased` / `domain_released` | `domain_id`,`pool` · — | §12.2 lease |
| `domain_pool_exhausted` / `domain_alloc_failed` | `pool` · — | lease failure |
| `process_running` | — (`pid`/`container`) | backend launched |
| `vecu_ready` / `verify_invoke` | — | lifecycle mirror |
| `cleanup_done` / `cleanup_noop` | — | teardown |
| `run_pass` | — | **terminal → PASSED** |
| `run_fail` | `verify_exit_code` | **terminal → FAILED** |
| `ready_timeout` / `start_failed` / `aborted_orphaned` | — · `reason` · `reason` | **terminal → ABORTED** |
| `orphan_detected` | `reason` | reap |
| `fmu_gate_allowed` / `fmu_gate_denied` | `fmu` | §5.6 FMU preflight |
| `fleet_start` / `fleet_ready` / `fleet_start_failed` / `member_start` | `domain_id`/`backend`/`members` · `members` · — · `instance_id`/`index` | fleet (§4.10) |

## Validate a trace

```bash
python3 tools/sils/conformance/validate_trace.py <trace.jsonl>          # lenient (raw stdout OK)
python3 tools/sils/conformance/validate_trace.py <trace.jsonl> --strict # every line must be an event
cat trace.jsonl | python3 .../validate_trace.py - --producer vecu       # only vecu events
```
Fail-closed: any violation → exit 1. See [conformance README](../tools/sils/conformance/README.md).

## Versioning

- `schema_version` is an integer; this is **1**.
- **Backward-compatible** (no version bump): adding a new event; adding an *optional*
  field to an existing event. Consumers ignore unknown fields/events (with
  `--allow-unknown-events`).
- **Breaking** (bump to 2): renaming/removing an event; adding/renaming a *required*
  field; changing a field's type or meaning. A v2 schema file is added alongside v1;
  `validate_trace.py --schema` selects which to check against.
