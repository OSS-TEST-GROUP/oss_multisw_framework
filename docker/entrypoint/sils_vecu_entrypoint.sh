#!/bin/bash
# Container entrypoint for runtime-mmf-sils-vecu (SiLS vECU Phase 1 PoC).
#
# Unlike core_entrypoint.sh, this container's whole lifetime IS the probe
# process -- no supervised multi-service fan-out. `exec` replaces this shell
# with the binary as PID 1 so `docker stop` delivers SIGTERM directly to it,
# which viss_probe_main.cc already handles (drains its wall-clock loop and
# runs the same participant/topic/publisher cleanup order as dds_transport.cpp).
#
# All behavior is env-var driven (see viss_probe_main.cc GetEnvOr calls) --
# VISS_DDS_DOMAIN_ID in particular MUST match the run's SUT domain (architecture
# v3 Final §3.4/§18-B); it is intentionally NOT defaulted here so a missing
# `-e VISS_DDS_DOMAIN_ID=...` at `docker run` time is caught by the probe's own
# "not set, using default domain 0" log line rather than silently masked.

set -u

export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-/oss/lib}"

if [ ! -x /oss/bin/sils_vecu_viss_probe ]; then
    echo "[sils_vecu_entrypoint] FATAL: /oss/bin/sils_vecu_viss_probe missing" >&2
    exit 1
fi

exec /oss/bin/sils_vecu_viss_probe
