#!/usr/bin/env python3
"""FMU security gate -- only allowlisted, hash-verified FMUs may be loaded (§5.6).

A `.fmu` is a ZIP that ships a native `.so`/`.dll`; instantiating it runs that code
in-process. Loading an untrusted or tampered FMU is therefore arbitrary code
execution (FMU guide docs/sils/sils-fmu-plant.md §5.6). This gate is the pre-load check
the Run Coordinator runs BEFORE FmuPlant ever opens the file:

  1. compute the FMU's sha256 over its exact bytes
  2. require a matching entry in the allowlist (config/sils/fmu/allowlist.yaml)
  3. if the topology named a specific entry (--expect-id), bind that id to the bytes
     approved for it -- a different-but-allowlisted FMU cannot stand in
  4. on allow, emit a provenance record (§5.10) so a Golden run is reproducible

Deny is fail-closed: unknown file, hash miss, or id/hash mismatch all exit 1 and the
Run Coordinator must not launch the FMU vECU. Metadata drift between the allowlist and
the (hash-verified) modelDescription is reported as a warning, not a denial -- the
bytes are what run, so a stale note never blocks, but it is surfaced.

Usage:
  fmu_gate.py --fmu model.fmu                          # gate against default allowlist
  fmu_gate.py --fmu model.fmu --expect-id vehicle-dyn  # bind to a named entry
  fmu_gate.py --fmu model.fmu --allowlist my.yaml --json      # emit provenance JSON
  fmu_gate.py --fmu model.fmu --manifest run-provenance.yaml  # write provenance file
  fmu_gate.py --fmu trusted.fmu --record              # print an allowlist stanza to add
Exit: 0 allow, 1 deny (security), 2 usage/IO.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "topology_compiler"))
import compile as tc  # noqa: E402  (reuse repo_root/load_yaml)

DEFAULT_ALLOWLIST = "config/sils/fmu/allowlist.yaml"


def sha256_of(path: Path) -> str:
    """Streaming sha256 over the whole .fmu file (bytes are the identity)."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_model_description(fmu: Path) -> tuple[dict, str | None]:
    """Pull the reproducibility-relevant facts out of the FMU's modelDescription.xml:
    fmi_version + model_identifier (the CoSimulation binary's identifier). Returns
    ({}, error) if the FMU has no readable modelDescription."""
    try:
        with zipfile.ZipFile(fmu) as z:
            xml = z.read("modelDescription.xml").decode("utf-8")
    except (zipfile.BadZipFile, KeyError, OSError) as e:
        return {}, f"cannot read modelDescription.xml from {fmu}: {e}"
    try:
        root = ET.fromstring(xml)
    except ET.ParseError as e:
        return {}, f"malformed modelDescription.xml in {fmu}: {e}"
    info = {"fmi_version": root.get("fmiVersion", "")}
    cs = root.find("CoSimulation")
    # FMI 2.0 puts modelIdentifier on <CoSimulation>; 3.0 keeps it there too.
    if cs is not None and cs.get("modelIdentifier"):
        info["model_identifier"] = cs.get("modelIdentifier")
    elif root.get("modelName"):
        info["model_identifier"] = root.get("modelName")
    return info, None


def load_allowlist(path: Path) -> tuple[list[dict], str | None]:
    if not path.exists():
        return [], (f"allowlist not found: {path} -- create it (see --record) so only "
                    "vetted FMUs can run; a missing allowlist denies every FMU (§5.6)")
    try:
        data = tc.load_yaml(path) or {}
    except Exception as e:  # noqa: BLE001
        return [], f"cannot parse allowlist {path}: {e}"
    fmus = data.get("fmus")
    if not isinstance(fmus, list):
        return [], f"allowlist {path}: top-level 'fmus' must be a list"
    return fmus, None


def gate(actual_sha: str, md: dict, allowlist: list[dict],
         expect_id: str | None) -> tuple[bool, list[str], list[str], dict | None]:
    """Return (allowed, reasons, warnings, matched_entry).

    Deny reasons are fail-closed. Warnings never flip an allow to deny; they flag
    allowlist metadata that drifted from the hash-verified modelDescription."""
    reasons: list[str] = []
    warnings: list[str] = []

    by_sha = {str(e.get("sha256", "")).lower(): e for e in allowlist if e.get("sha256")}

    if expect_id is not None:
        # Named binding: the entry the topology asked for must exist AND its approved
        # bytes must be exactly these bytes. This stops an allowlisted FMU 'A' from
        # being served where the topology expected allowlisted FMU 'B'.
        entry = next((e for e in allowlist if e.get("id") == expect_id), None)
        if entry is None:
            reasons.append(f"--expect-id '{expect_id}' has no entry in the allowlist")
            return False, reasons, warnings, None
        approved = str(entry.get("sha256", "")).lower()
        if approved != actual_sha:
            reasons.append(f"FMU sha256 {actual_sha} does not match the bytes approved for "
                           f"'{expect_id}' (allowlist: {approved or 'none'}) -- tampered or wrong file")
            return False, reasons, warnings, None
        matched = entry
    else:
        matched = by_sha.get(actual_sha)
        if matched is None:
            reasons.append(f"FMU sha256 {actual_sha} is not in the allowlist -- refusing to "
                           "load unvetted native code (§5.6). Vet it, then add via --record")
            return False, reasons, warnings, None

    # Allowed. Cross-check hash-verified modelDescription against allowlist metadata.
    for field in ("fmi_version", "model_identifier"):
        want = matched.get(field)
        got = md.get(field)
        if want is not None and got is not None and str(want) != str(got):
            warnings.append(f"allowlist entry '{matched.get('id')}' {field}={want!r} but the FMU "
                            f"declares {got!r} (allowlist note is stale; bytes verified)")
    return True, reasons, warnings, matched


def build_provenance(fmu: Path, actual_sha: str, md: dict, entry: dict) -> dict:
    """The reproducibility record (§5.10). Fields the gate can derive from the file +
    allowlist; binary/parameter sub-hashes and importer commit are filled by the build
    system and left absent here rather than faked."""
    prov = {
        "fmu": {
            "file": fmu.name,
            "sha256": actual_sha,
            "size_bytes": fmu.stat().st_size,
            "fmi_version": md.get("fmi_version", ""),
            "model_identifier": md.get("model_identifier", ""),
        },
        "allowlist": {
            "id": entry.get("id"),
            "supplier": entry.get("supplier"),
        },
    }
    return prov


def to_yaml(prov: dict) -> str:
    """Minimal YAML dumper for the flat 2-level provenance dict (no PyYAML dependency
    at emit time; keeps the manifest stable and diff-friendly)."""
    lines: list[str] = []
    for section, body in prov.items():
        lines.append(f"{section}:")
        for k, v in body.items():
            val = "" if v is None else v
            if isinstance(val, str) and (val == "" or ":" in val or val.startswith(" ")):
                val = f'"{val}"'
            lines.append(f"  {k}: {val}")
    return "\n".join(lines) + "\n"


def record_stanza(actual_sha: str, md: dict) -> str:
    """A ready-to-paste allowlist entry for a maintainer who has vetted this FMU."""
    ident = md.get("model_identifier") or "UNNAMED"
    slug = ident.lower().replace(" ", "-")
    return (
        "  - id: " + slug + "\n"
        f"    sha256: \"{actual_sha}\"\n"
        f"    fmi_version: \"{md.get('fmi_version', '')}\"\n"
        f"    model_identifier: \"{ident}\"\n"
        "    supplier: \"FILL-IN provenance: where this FMU came from + who vetted it\"\n"
        "    platforms: [aarch64-linux]\n"
    )


def main(argv: list[str] | None = None) -> int:
    root = tc.repo_root()
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--fmu", required=True, help="path to the .fmu to gate")
    parser.add_argument("--allowlist", default=DEFAULT_ALLOWLIST,
                        help=f"allowlist YAML (relative to repo root; default {DEFAULT_ALLOWLIST})")
    parser.add_argument("--repo-root", type=Path, default=root)
    parser.add_argument("--expect-id", default=None,
                        help="require the FMU to be the allowlist entry with this id")
    parser.add_argument("--json", action="store_true", help="emit provenance as JSON")
    parser.add_argument("--manifest", default=None, help="write provenance YAML to this path")
    parser.add_argument("--record", action="store_true",
                        help="print an allowlist stanza for this (trusted) FMU and exit")
    args = parser.parse_args(argv)

    fmu = Path(args.fmu)
    if not fmu.exists():
        print(f"error: fmu not found: {fmu}", file=sys.stderr)
        return 2

    actual_sha = sha256_of(fmu)
    md, md_err = read_model_description(fmu)
    if md_err:
        # A file we cannot parse as an FMU is not something we will vouch for.
        print(f"error: {md_err}", file=sys.stderr)
        return 2

    if args.record:
        sys.stdout.write(record_stanza(actual_sha, md))
        return 0

    allowlist_path = (args.repo_root / args.allowlist
                      if not Path(args.allowlist).is_absolute() else Path(args.allowlist))
    allowlist, al_err = load_allowlist(allowlist_path)
    if al_err:
        print(f"error: {al_err}", file=sys.stderr)
        return 1  # missing/broken allowlist denies (fail-closed)

    allowed, reasons, warnings, entry = gate(actual_sha, md, allowlist, args.expect_id)

    for w in warnings:
        print(f"warning: {w}", file=sys.stderr)
    if not allowed:
        for r in reasons:
            print(f"DENY: {r}", file=sys.stderr)
        print(f"fmu gate DENY: {fmu.name} ({actual_sha[:16]}...)", file=sys.stderr)
        return 1

    prov = build_provenance(fmu, actual_sha, md, entry)
    if args.manifest:
        Path(args.manifest).write_text(to_yaml(prov), encoding="utf-8")
        print(f"# provenance written to {args.manifest}", file=sys.stderr)
    if args.json:
        print(json.dumps(prov))
    else:
        print("RESOLVED_FMU_GATE=allow")
        print(f"RESOLVED_FMU_SHA256={actual_sha}")
        print(f"RESOLVED_FMU_ALLOWLIST_ID={entry.get('id')}")
    print(f"fmu gate ALLOW: {fmu.name} = '{entry.get('id')}' ({actual_sha[:16]}...)",
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
