#!/usr/bin/env python3
"""
Generate ROS2 .msg files from DDS IDL files.
Run this script when DDS IDL files change.

Usage:
    python3 idl_to_msg.py --idl-root ../../dds --output-dir ./msg
"""

import argparse
import re
import sys
from pathlib import Path


# ── Type mapping: DDS IDL → ROS2 .msg ──────────────────────────────────────
TYPE_MAP = {
    "long long":  "int64",
    "long":       "int32",
    "int32":      "int32",
    "int64":      "int64",
    "short":      "int16",
    "octet":      "uint8",
    "boolean":    "bool",
    "float":      "float32",
    "double":     "float64",
    "string":     "string",
}

# sequence<X> → X[]
SEQUENCE_RE = re.compile(r"sequence\s*<\s*([^>]+?)\s*>")

# Known enum types defined in DDS IDL → mapped to int32 in .msg
# (module, enum_name)
KNOWN_ENUMS = {
    "ReturnValue", "CommonStatusType",
    "DiagnosticStatusType",
    "ServiceType", "RecoveryPolicy", "ReturnCode",
    "ServiceMode", "ServiceState", "OsType",
}

# Nested message types that become mmf_interfaces/TypeName in .msg
NESTED_MSG_TYPES = {
    "Service":       "mmf_interfaces/LifecycleService",
    "ServiceStatus": "mmf_interfaces/ServiceStatus",
}

# Rename DDS struct names → ROS2 .msg names (to avoid collisions or for clarity)
STRUCT_RENAME = {
    "Service": "LifecycleService",
}

# Struct names to skip entirely (placeholder/dummy types not used in messaging)
SKIP_STRUCTS = {
    "DummyType2", "DummyTypeT", "DiagnosticCommonDummy",
    "ReturnValueStruct",  # ReturnValue enum is handled as int32
}


def map_type(dds_type: str) -> str:
    """Convert a DDS IDL type to ROS2 .msg type."""
    dds_type = dds_type.strip()

    # sequence<X> → X[]
    m = SEQUENCE_RE.match(dds_type)
    if m:
        inner = map_type(m.group(1))
        return inner + "[]"

    # Known nested message types
    if dds_type in NESTED_MSG_TYPES:
        return NESTED_MSG_TYPES[dds_type]

    # Known enums → int32
    if dds_type in KNOWN_ENUMS:
        return "int32"

    # Primitive types
    for dds, ros in TYPE_MAP.items():
        if dds_type == dds:
            return ros

    # Fallback: return as-is (warn)
    print(f"  [WARN] Unknown type: '{dds_type}' → kept as-is", file=sys.stderr)
    return dds_type


def parse_idl_file(idl_path: Path):
    """
    Parse a DDS IDL file and return a list of (struct_name, fields, module_name) tuples.
    fields: list of (ros2_type, field_name)
    """
    text = idl_path.read_text()

    # Strip C/C++ comments
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)

    # Remove #include lines
    text = re.sub(r"#include\s+[^\n]+", "", text)

    results = []

    # Extract module name
    module_m = re.search(r"module\s+(\w+)", text)
    module_name = module_m.group(1) if module_m else ""

    # Extract all struct definitions
    struct_re = re.compile(
        r"struct\s+(\w+)\s*\{([^}]*)\}", re.DOTALL
    )
    for m in struct_re.finditer(text):
        struct_name = m.group(1)
        body = m.group(2)

        # Skip dummy/placeholder structs
        if struct_name in SKIP_STRUCTS:
            continue

        # Apply rename mapping
        struct_name = STRUCT_RENAME.get(struct_name, struct_name)

        # Empty body → empty message (e.g. NotifyOtaManagerReady)
        if body.strip() == "":
            fields = []  # empty message
        else:
            fields = []
            for line in body.splitlines():
                line = line.strip().rstrip(";").strip()
                if not line:
                    continue
                # Remove @key annotation
                line = re.sub(r"@key\s+", "", line)
                # Split type and name (last token is name)
                parts = line.rsplit(None, 1)
                if len(parts) == 2:
                    dds_type, field_name = parts
                    ros_type = map_type(dds_type.strip())
                    fields.append((ros_type, field_name))

        results.append((struct_name, fields, module_name, idl_path))

    return results


def generate_msg(struct_name, fields, module_name, idl_path, output_dir: Path):
    """Generate a single .msg file."""
    lines = []
    lines.append(f"# Auto-generated from {idl_path.relative_to(idl_path.parents[3])}")
    lines.append(f"# Module: {module_name}, Struct: {struct_name}")
    if not fields:
        lines.append("# Empty message")
    else:
        for ros_type, field_name in fields:
            lines.append(f"{ros_type} {field_name}")

    out_file = output_dir / f"{struct_name}.msg"
    content = "\n".join(lines) + "\n"

    # Only write if changed
    if out_file.exists() and out_file.read_text() == content:
        return False

    out_file.write_text(content)
    print(f"  [GEN] {out_file.name}")
    return True


def main():
    parser = argparse.ArgumentParser(description="Convert DDS IDL to ROS2 .msg files")
    parser.add_argument("--idl-root", required=True, help="Root directory of DDS IDL files")
    parser.add_argument("--output-dir", required=True, help="Output directory for .msg files")
    parser.add_argument("--dry-run", action="store_true", help="Print what would be generated")
    args = parser.parse_args()

    idl_root = Path(args.idl_root).resolve()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    # IDL files to process (skip agl/ros2/Gid* which are rmw_dds_common types)
    skip_patterns = {"String_.idl", "Gid.idl", "NodeEntitiesInfo.idl", "ParticipantEntitiesInfo.idl"}

    idl_files = [f for f in idl_root.rglob("*.idl") if f.name not in skip_patterns]

    generated = []
    for idl_path in sorted(idl_files):
        print(f"[IDL] {idl_path.relative_to(idl_root)}")
        structs = parse_idl_file(idl_path)
        for struct_name, fields, module_name, src in structs:
            if args.dry_run:
                print(f"  → {struct_name}.msg ({len(fields)} fields)")
            else:
                generate_msg(struct_name, fields, module_name, src, output_dir)
            generated.append(struct_name)

    print(f"\nTotal: {len(generated)} .msg files")


if __name__ == "__main__":
    main()
