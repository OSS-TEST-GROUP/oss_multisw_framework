#!/bin/bash
# Container entrypoint for the brake-ecu example image.
#
# Structurally identical to docker/entrypoint/sils_vecu_entrypoint.sh -- the ONLY
# difference is the binary name (brake_ecu instead of sils_vecu_viss_probe). This is
# the whole "packaging" a developer ECU needs: exec the ECU binary as PID 1 so
# `docker stop` delivers SIGTERM straight to it (RunProbe's signal handler drains the
# loop and tears down the participant cleanly). All behavior is env-driven -- the run's
# SILS_* / BRAKE_* and VISS_DDS_DOMAIN_ID are injected at `docker run` time.
set -u

export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-/oss/lib}"

if [ ! -x /oss/bin/brake_ecu ]; then
    echo "[brake-ecu entrypoint] FATAL: /oss/bin/brake_ecu missing" >&2
    exit 1
fi

exec /oss/bin/brake_ecu
