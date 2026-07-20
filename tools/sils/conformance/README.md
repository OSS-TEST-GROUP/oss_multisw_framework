# SiLS Conformance Kit

Checks a vECU **run trace** against the published boundary — the
[trace schema](../../../docs/sils-trace-schema.md) and the fidelity/parity contracts.
It consumes only the JSONL trace (`docs/sils-extension-model.md §2`), so it runs
**outside** the vECU: no build, no DDS, no source — just Python 3 and a captured
`trace.jsonl`. A developer runs it on their own ECU's output; CI runs the same tools.

## The gate (one command)

**`gate.py`** is the single fail-closed entry point: it chains the checkers below in order
(schema → fidelity → optional parity) and exits non-zero if any stage fails. Run this in
CI or before you push; reach for the individual tools only to compose a custom flow.

```bash
python3 gate.py path/to/trace.jsonl                       # schema + fidelity
python3 gate.py runs/my-run/ --resolved runs/my-run/resolved-run.yaml --junit out.xml
python3 gate.py sils.jsonl --target target.jsonl --contract parity.yaml   # + L4 parity
```

A committed sample — [`samples/nominal.trace.jsonl`](samples/nominal.trace.jsonl), a full
merged run trace — is the gate's smoke fixture (and what CI runs). `.github/workflows/
sils-conformance.yml` runs the unit tests + this gate on every push/PR touching the kit.

## The three checkers it chains

| tool | question it answers | needs |
|---|---|---|
| **`validate_trace.py`** | Is the trace well-formed against the schema? (events known, required fields present) | a trace |
| **`evaluate.py`** | What fidelity level (L0–L4) does the run demonstrate, and is it conformant? | trace(s), optional `resolved-run.yaml` |
| **`parity.py`** | Does the SiLS run match a real-target reference within the parity contract? (L4) | SiLS trace, target trace, parity contract |

`compare_golden.py` / `capture_golden.py` back regression comparison (record a golden
trace, diff a new run against it); `to_junit.py` renders results for CI.

## Run it

```bash
# 1) schema conformance — the first gate
python3 validate_trace.py path/to/trace.jsonl
python3 validate_trace.py path/to/trace.jsonl --strict         # every line must be an event
cat trace.jsonl | python3 validate_trace.py - --producer vecu  # only the vECU's own events

# 2) fidelity evaluation (one run, or a whole run dir of *.trace.jsonl)
python3 evaluate.py path/to/trace.jsonl
python3 evaluate.py runs/my-run/ --resolved runs/my-run/resolved-run.yaml

# 3) target parity (L4)
python3 parity.py --sils sils-run.jsonl --target target-run.jsonl \
        --contract ../../../config/sils/parity/viss-target-parity.yaml
```

All three are **fail-closed**: any violation → non-zero exit, so they drop straight into
a CI gate or a pre-merge check.

## Typical flow for a developer ECU

```bash
# capture your ECU's trace (any environment that runs the container/binary)
<your-ecu-binary>  > my-ecu.trace.jsonl        # stdout is the trace (stderr noise is fine)

# gate it locally before you push
python3 validate_trace.py my-ecu.trace.jsonl   # schema OK?
python3 evaluate.py       my-ecu.trace.jsonl   # what fidelity did it earn?
```

Because the kit only needs Python + the trace, it packages as a standalone checker:
copy `tools/sils/conformance/` + `config/sils/trace-schema/` (and, for parity, your
`config/sils/parity/*.yaml`), or run it from a minimal `python:3` container mounting the
trace. No part of the vECU build is required.

## Tests

```bash
python3 -m pytest tools/sils/conformance/        # validate/evaluate/parity/compare/junit
```
