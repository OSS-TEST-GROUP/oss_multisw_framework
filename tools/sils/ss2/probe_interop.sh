#!/usr/bin/env bash
#
# B1-3 -- measure the ROS 2 side of the loop-B bridge so BR-C-2a stops being a
# table of open questions.
#
# Run this INSIDE the SS_v2 container (docker/sils/Dockerfile.ss2 bakes it in at
# /opt/sils/probe_interop.sh) while a simulator is running on the same domain.
#
# What it collects, and why each item is in BR-C-2a:
#
#   RMW implementation      which vendor the ROS 2 side actually loaded (upstream
#                           defaults to Cyclone; our layer defaults to Fast DDS --
#                           run this twice, once per value, to fill both columns)
#   ROS_DOMAIN_ID           ROS 2 maps this 1:1 onto the DDS domain id, so it must
#                           equal our VISS_DDS_DOMAIN_ID or nothing meets
#   topic list + types      the ROS message types the adapter has to translate
#   per-endpoint QoS        reliability/durability/history/deadline/liveliness --
#                           the RxO matrix. A mismatch here is a SILENT no-match,
#                           which is why it must be measured and not assumed.
#   derived DDS names       ROS 2 mangles names: topic `/foo` -> `rt/foo`, type
#                           `pkg/msg/Bar` -> `pkg::msg::dds_::Bar_`. Our pure-DDS
#                           vECU sees the MANGLED names, which is exactly why a
#                           bridge exists at all rather than a direct subscribe.
#
# NOT collected -- stated plainly rather than guessed:
#   XCDR version (XCDR1 vs XCDR2). No ros2 CLI verb exposes the negotiated
#   data_representation. Determine it with an RTPS capture instead:
#       tcpdump -i any -w rtps.pcap 'udp portrange 7400-7500'
#   then read the DATA submessage encapsulation id in Wireshark's RTPS dissector
#   (0x0000/0x0001 = CDR/XCDR1, 0x0006/0x0007 = XCDR2). See sils-ss2-bringup.md.
#
# Usage (inside the container):
#   /opt/sils/probe_interop.sh
#   /opt/sils/probe_interop.sh --wait 60        # wait up to 60s for topics
#   SILS_PROBE_OUT=/out /opt/sils/probe_interop.sh
#
# Exit codes: 0 report written  ·  1 usage/environment error  ·  3 no topics found
# Docs: docs/sils/sils-ss2-bringup.md · docs/sils/sils-real-bridge-requirements.md (BR-C-2a)

set -euo pipefail

OUT_DIR="${SILS_PROBE_OUT:-/out}"
WAIT_SEC=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --wait) WAIT_SEC="${2:?--wait needs seconds}"; shift 2 ;;
        --out)  OUT_DIR="${2:?--out needs a dir}"; shift 2 ;;
        -h|--help) sed -n '2,40p' "$0"; exit 0 ;;
        *) echo "unknown arg: $1" >&2; exit 1 ;;
    esac
done

command -v ros2 >/dev/null || { echo "ERROR: ros2 not on PATH -- source the overlay first" >&2; exit 1; }

mkdir -p "${OUT_DIR}/raw"
raw="${OUT_DIR}/raw"

echo "==> probing (out: ${OUT_DIR})"

# --- environment -------------------------------------------------------------
# Captured verbatim: a QoS number is only meaningful next to the RMW that produced it.

{
    echo "rmw_implementation=${RMW_IMPLEMENTATION:-<unset:ros2 default>}"
    echo "ros_domain_id=${ROS_DOMAIN_ID:-<unset:0>}"
    echo "ros_distro=${ROS_DISTRO:-<unset>}"
    echo "ros_localhost_only=${ROS_LOCALHOST_ONLY:-<unset>}"
    echo "fastdds_profiles=${FASTDDS_DEFAULT_PROFILES_FILE:-<unset>}"
    echo "cyclonedds_uri=${CYCLONEDDS_URI:-<unset>}"
    echo "arch=$(uname -m)"
    echo "kernel=$(uname -r)"
} > "${raw}/env.txt"

ros2 doctor --report > "${raw}/doctor.txt" 2>&1 || echo "(ros2 doctor failed)" >> "${raw}/doctor.txt"

# --- wait for the simulator to publish ---------------------------------------
# A probe that reports "0 topics" because it raced the simulator is worse than
# one that waits: the empty result looks like a discovery failure.

deadline=$(( $(date +%s) + WAIT_SEC ))
while :; do
    ros2 topic list -t > "${raw}/topics.txt" 2>/dev/null || : > "${raw}/topics.txt"
    # `ros2 topic list` always reports /parameter_events and /rosout; those are the
    # CLI node's own, not the simulator's, so they do not count as "found".
    n=$(grep -cvE '^/(parameter_events|rosout) ' "${raw}/topics.txt" || true)
    [[ "${n}" -gt 0 ]] && break
    [[ "$(date +%s)" -ge "${deadline}" ]] && break
    sleep 2
done

ros2 node list > "${raw}/nodes.txt" 2>/dev/null || : > "${raw}/nodes.txt"

# --- per-topic QoS -----------------------------------------------------------

: > "${raw}/topic_info.txt"
while read -r line; do
    [[ -z "${line}" ]] && continue
    topic="${line%% *}"
    {
        echo "### ${topic}"
        ros2 topic info -v "${topic}" 2>&1 || echo "(topic info failed)"
        echo
    } >> "${raw}/topic_info.txt"
done < "${raw}/topics.txt"

# --- assemble ----------------------------------------------------------------
# Python does the parsing: getting the endpoint/QoS blocks right in awk is the
# kind of thing that silently drops a field.

python3 - "${OUT_DIR}" <<'PYEOF'
import json, os, re, sys

out = sys.argv[1]
raw = os.path.join(out, "raw")

def read(name):
    p = os.path.join(raw, name)
    return open(p, encoding="utf-8", errors="replace").read() if os.path.exists(p) else ""

env = dict(
    line.split("=", 1)
    for line in read("env.txt").splitlines() if "=" in line
)

# `ros2 topic list -t` prints:  /topic [pkg/msg/Type]
topics = []
for line in read("topics.txt").splitlines():
    m = re.match(r"^(\S+)\s+\[(.+)\]\s*$", line.strip())
    if m:
        topics.append({"topic": m.group(1), "ros_type": m.group(2)})

BARE_DDS = "_CREATED_BY_BARE_DDS_APP_"

def dds_names(topic, ros_type):
    """ROS 2 name mangling -- what a pure-DDS participant actually sees.

    Convention, not a measurement: rmw_fastrtps/rmw_cyclonedds both apply it, but
    it is derived here rather than read off the wire. Confirm against an RTPS
    capture before writing it into BR-C-2a as fact.

    Note this is safer than it looks for anything `ros2 topic list` returns at all:
    that verb demangles, so a topic only shows up if it already carried the `rt/`
    prefix. A bare DDS app publishing an unprefixed topic is invisible here --
    absence from this table is NOT evidence the topic does not exist.
    """
    parts = ros_type.split("/")           # pkg/msg/Bar
    if len(parts) == 3:
        pkg, kind, msg = parts
        dds_type = f"{pkg}::{kind}::dds_::{msg}_"
    else:
        dds_type = f"<unparsed:{ros_type}>"
    return {"dds_topic": "rt" + topic, "dds_type": dds_type}

for t in topics:
    t.update(dds_names(t["topic"], t["ros_type"]))

# Split topic_info.txt into per-topic endpoint records.
endpoints = []
cur_topic = None
cur = None
QOS_KEYS = {
    "Reliability": "reliability",
    "Durability": "durability",
    "History (Depth)": "history",
    "Lifespan": "lifespan",
    "Deadline": "deadline",
    "Liveliness": "liveliness",
    "Liveliness lease duration": "liveliness_lease",
}
for line in read("topic_info.txt").splitlines():
    s = line.strip()
    if line.startswith("### "):
        # Flush before switching topics: the last endpoint of a block has no
        # following "Node name:" to trigger the flush below, so dropping it here
        # silently loses one endpoint per topic.
        if cur:
            endpoints.append(cur)
        cur_topic = line[4:].strip()
        cur = None
        continue
    if s.startswith("Node name:"):
        if cur:
            endpoints.append(cur)
        cur = {"topic": cur_topic, "node": s.split(":", 1)[1].strip(), "qos": {}}
        continue
    if cur is None:
        continue
    if s.startswith("Endpoint type:"):
        cur["endpoint"] = s.split(":", 1)[1].strip()
    elif s.startswith("Topic type:"):
        cur["ros_type"] = s.split(":", 1)[1].strip()
    else:
        for label, key in QOS_KEYS.items():
            if s.startswith(label + ":"):
                cur["qos"][key] = s.split(":", 1)[1].strip()
                break
if cur:
    endpoints.append(cur)

# --- domain hygiene ----------------------------------------------------------
# Endpoints reported as _CREATED_BY_BARE_DDS_APP_ are DDS participants with no ROS
# node metadata -- i.e. NOT the simulator. Finding them means this DDS domain is
# shared with something else. That matters twice over:
#   - a SiLS golden/regression run on a shared domain can match foreign endpoints,
#     which is the same class of silent-wrong-verdict the plant isolation gate exists
#     to prevent;
#   - conversely, seeing them at all PROVES bare-DDS <-> ROS 2 discovery works on
#     this domain, which is the central BR-C-2a question.
foreign = sorted({e["topic"] for e in endpoints if e.get("node") == BARE_DDS})
sim_nodes = sorted({e["node"] for e in endpoints
                    if e.get("node") not in (BARE_DDS, None)
                    and not e.get("node", "").startswith("_ros2cli_daemon")})

# rmw_fastrtps does not report History through `ros2 topic info -v`; it comes back
# UNKNOWN for every endpoint. Say so instead of leaving a column of UNKNOWN that
# reads like a measurement.
histories = {e.get("qos", {}).get("history") for e in endpoints}
history_unavailable = bool(endpoints) and histories <= {"UNKNOWN", None}

not_measured = {
    "xcdr_version": "no ros2 CLI verb exposes negotiated data_representation; "
                    "use an RTPS capture (see script header)",
    "discovery_transport": "infer from the RTPS capture / profile file, not from ros2 CLI",
}
if history_unavailable:
    not_measured["history"] = ("every endpoint reported History=UNKNOWN -- this is an "
                               "rmw reporting gap, not a QoS value. Read depth from the "
                               "publisher's source/profile instead.")

report = {
    "schema": "sils-ss2-interop-probe/v1",
    "purpose": "BR-C-2a input (B1-3)",
    "environment": env,
    "counts": {"topics": len(topics), "endpoints": len(endpoints),
               "nodes": len([n for n in read("nodes.txt").splitlines() if n.strip()])},
    "domain_sharing": {
        "bare_dds_topics": foreign,
        "ros_nodes": sim_nodes,
        "shared": bool(foreign),
    },
    "topics": topics,
    "endpoints": endpoints,
    "not_measured": not_measured,
}

with open(os.path.join(out, "interop-probe.json"), "w", encoding="utf-8") as f:
    json.dump(report, f, indent=2, ensure_ascii=False)
    f.write("\n")

# Markdown for pasting straight into BR-C-2a.
L = []
L.append("# B1-3 interop probe — ROS 2 side\n")
L.append("> Generated by `tools/sils/ss2/probe_interop.sh`. "
         "Paste into `docs/sils/sils-real-bridge-requirements.md` (BR-C-2a).\n")
L.append("## Environment\n")
L.append("| key | value |")
L.append("|---|---|")
for k, v in env.items():
    L.append(f"| `{k}` | `{v}` |")
L.append("")
L.append(f"## Topics ({len(topics)})\n")
if topics:
    L.append("| ROS topic | ROS type | DDS topic (derived) | DDS type name (derived) |")
    L.append("|---|---|---|---|")
    for t in topics:
        L.append(f"| `{t['topic']}` | `{t['ros_type']}` | `{t['dds_topic']}` | `{t['dds_type']}` |")
else:
    L.append("_none — simulator not running, or wrong ROS_DOMAIN_ID._")
L.append("")
L.append(f"## Endpoint QoS ({len(endpoints)})\n")
if endpoints:
    L.append("| topic | endpoint | node | reliability | durability | history | deadline | liveliness |")
    L.append("|---|---|---|---|---|---|---|---|")
    for e in endpoints:
        q = e.get("qos", {})
        L.append("| `{}` | {} | `{}` | {} | {} | {} | {} | {} |".format(
            e.get("topic", "?"), e.get("endpoint", "?"), e.get("node", "?"),
            q.get("reliability", "-"), q.get("durability", "-"), q.get("history", "-"),
            q.get("deadline", "-"), q.get("liveliness", "-")))
else:
    L.append("_none._")
L.append("")
L.append("## Domain sharing\n")
L.append(f"ROS 2 nodes seen: {', '.join('`%s`' % n for n in sim_nodes) or '_none_'}\n")
if foreign:
    L.append("> ⚠ **This DDS domain is shared with non-ROS participants.** The endpoints "
             "below report `_CREATED_BY_BARE_DDS_APP_`, meaning a pure DDS application "
             "(not the simulator) is on the same domain.\n")
    L.append("> Two consequences, opposite in sign:\n")
    L.append("> - **Good:** bare-DDS ↔ ROS 2 discovery demonstrably works here — the "
             "central BR-C-2a question, answered affirmatively for this RMW.")
    L.append("> - **Bad:** a SiLS golden/regression run on a shared domain can match "
             "foreign endpoints. Give loop B its own domain id before capturing goldens.\n")
    for t in foreign:
        L.append(f"> - `{t}`")
    L.append("")
else:
    L.append("_No non-ROS participants detected on this domain._\n")
L.append("## Not measured here\n")
for k, v in report["not_measured"].items():
    L.append(f"- **{k}** — {v}")
L.append("")

with open(os.path.join(out, "interop-probe.md"), "w", encoding="utf-8") as f:
    f.write("\n".join(L))

print(f"topics={len(topics)} endpoints={len(endpoints)}")
PYEOF

echo "==> wrote ${OUT_DIR}/interop-probe.json"
echo "==> wrote ${OUT_DIR}/interop-probe.md"

# Distinguish "probe ran, nothing there" from "probe worked". An empty result is
# almost always a domain-id or discovery problem, not an absent simulator.
if ! grep -qvE '^/(parameter_events|rosout) ' "${raw}/topics.txt" 2>/dev/null; then
    cat >&2 <<EOF

WARNING: no simulator topics found.
  Check, in this order:
    1. is a scenario actually running?
    2. ROS_DOMAIN_ID (${ROS_DOMAIN_ID:-unset->0}) == the vECU's VISS_DDS_DOMAIN_ID?
    3. network_mode: host on every participant?
    4. RMW mismatch -- Cyclone and Fast DDS do not discover each other.
EOF
    exit 3
fi
