#!/usr/bin/env python3
"""MMF integration smoke test (L0, x86_64).

Drives the full monitor-facing path through the running integration stack
(docker-compose-integration.yaml): lifecycle_mgr / OtaManager (mmf-core) →
dds_bridge → rosbridge_server (mmf-ext-ros2) → this client (the role a Flutter
monitor would play). Verifies the two periodic, always-on signals reach a
rosbridge WebSocket client:

  - lifecycle: LifecycleDiagReport          (StateMonitor periodic publish)
  - OTA:       NotifyOtaMgrHeartbeat         (diagnostic service periodic publish)

Exits 0 on success, non-zero on any missing signal — so a CI stage (Jenkins)
can gate on it. Pure stdlib + websocket-client.

Usage:
  python3 tests/integration/integration_smoke.py
  ROSBRIDGE_WS_URL=ws://127.0.0.1:9090 TIMEOUT_SEC=40 python3 tests/integration/integration_smoke.py
"""
import json
import os
import sys
import time

try:
    import websocket  # websocket-client
except ImportError:
    print("[smoke] FATAL: websocket-client not installed "
          "(pip3 install websocket-client)", file=sys.stderr)
    sys.exit(2)

URL = os.environ.get("ROSBRIDGE_WS_URL", "ws://127.0.0.1:9090")
TIMEOUT_SEC = int(os.environ.get("TIMEOUT_SEC", "40"))

# (label, topic, ROS msg type) — the periodic monitor-facing signals.
CHECKS = [
    ("lifecycle.LifecycleDiagReport",
     "/mmf/lifecyclemgr/LifecycleDiagReport", "lifecyclemgr/LifecycleDiagReport"),
    ("ota.NotifyOtaMgrHeartbeat",
     "/mmf/otamanager/diagnostic_service/NotifyOtaMgrHeartbeat",
     "otamanager/NotifyOtaMgrHeartbeat"),
]


def main() -> int:
    deadline = time.time() + TIMEOUT_SEC
    print(f"[smoke] connecting {URL} (timeout {TIMEOUT_SEC}s)")
    try:
        ws = websocket.create_connection(URL, timeout=10)
    except Exception as e:  # noqa: BLE001 — boundary: external connection
        print(f"[smoke] FATAL: cannot connect to rosbridge: {e}", file=sys.stderr)
        return 1

    for label, topic, typ in CHECKS:
        ws.send(json.dumps({"op": "subscribe", "topic": topic, "type": typ}))
    print(f"[smoke] subscribed to {len(CHECKS)} topics; waiting for samples...")

    received = {label: False for (label, _, _) in CHECKS}
    by_topic = {topic: label for (label, topic, _) in CHECKS}

    ws.settimeout(2)
    while time.time() < deadline and not all(received.values()):
        try:
            raw = ws.recv()
        except websocket.WebSocketTimeoutException:
            continue
        except Exception as e:  # noqa: BLE001
            print(f"[smoke] recv error: {e}", file=sys.stderr)
            break
        if not raw:
            continue
        msg = json.loads(raw)
        if msg.get("op") != "publish":
            continue
        label = by_topic.get(msg.get("topic"))
        if label and not received[label]:
            received[label] = True
            print(f"[smoke] OK  ← {label}")

    try:
        ws.close()
    except Exception:  # noqa: BLE001
        pass

    print("\n===== MMF INTEGRATION SMOKE =====")
    ok = True
    for label in received:
        status = "PASS" if received[label] else "FAIL"
        if not received[label]:
            ok = False
        print(f"  {status}  {label}")
    print("=================================")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
