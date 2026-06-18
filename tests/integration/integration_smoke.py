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
import xml.etree.ElementTree as ET

try:
    import websocket  # websocket-client
except ImportError:
    print("[smoke] FATAL: websocket-client not installed "
          "(pip3 install websocket-client)", file=sys.stderr)
    sys.exit(2)

URL = os.environ.get("ROSBRIDGE_WS_URL", "ws://127.0.0.1:9090")
TIMEOUT_SEC = int(os.environ.get("TIMEOUT_SEC", "40"))
# JUnit 포맷(언어 무관 표준) 결과 — Jenkins junit 스텝이 파싱. 단위(GoogleTest)와 동일 컨벤션.
JUNIT_OUT = os.environ.get("JUNIT_OUT", "output/integration-results/integration_smoke.xml")

# (label, topic, ROS msg type) — the periodic monitor-facing signals.
CHECKS = [
    ("lifecycle.LifecycleDiagReport",
     "/mmf/lifecyclemgr/LifecycleDiagReport", "lifecyclemgr/LifecycleDiagReport"),
    ("ota.NotifyOtaMgrHeartbeat",
     "/mmf/otamanager/diagnostic_service/NotifyOtaMgrHeartbeat",
     "otamanager/NotifyOtaMgrHeartbeat"),
]


def write_junit(received, elapsed, error=None):
    """신호별 결과를 JUnit 포맷 XML 로 기록한다 (stdlib only, Java 비의존).

    각 CHECK = testcase. 미수신/연결실패는 <failure>. 모든 종료 경로에서 호출해
    CI(Jenkins junit 스텝)가 항상 리포트를 받도록 한다.
    """
    topics = {label: topic for (label, topic, _) in CHECKS}
    failures = sum(1 for ok in received.values() if not ok)
    suite = ET.Element(
        "testsuite", name="mmf.integration.smoke",
        tests=str(len(received)), failures=str(failures), time="%.2f" % elapsed,
    )
    for label, ok in received.items():
        case = ET.SubElement(
            suite, "testcase", classname="mmf.integration.smoke",
            name=label, time="%.2f" % elapsed,
        )
        if not ok:
            msg = error or ("signal not received within %ds" % TIMEOUT_SEC)
            failure = ET.SubElement(case, "failure", message=msg)
            failure.text = "topic=%s" % topics.get(label, "?")
    os.makedirs(os.path.dirname(JUNIT_OUT) or ".", exist_ok=True)
    ET.ElementTree(suite).write(JUNIT_OUT, encoding="utf-8", xml_declaration=True)
    print("[smoke] JUnit report -> %s" % JUNIT_OUT)


def main() -> int:
    start = time.time()
    deadline = start + TIMEOUT_SEC
    received = {label: False for (label, _, _) in CHECKS}
    print(f"[smoke] connecting {URL} (timeout {TIMEOUT_SEC}s)")
    try:
        ws = websocket.create_connection(URL, timeout=10)
    except Exception as e:  # noqa: BLE001 — boundary: external connection
        print(f"[smoke] FATAL: cannot connect to rosbridge: {e}", file=sys.stderr)
        write_junit(received, time.time() - start,
                    error=f"cannot connect to rosbridge: {e}")
        return 1

    for label, topic, typ in CHECKS:
        ws.send(json.dumps({"op": "subscribe", "topic": topic, "type": typ}))
    print(f"[smoke] subscribed to {len(CHECKS)} topics; waiting for samples...")

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
    write_junit(received, time.time() - start)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
