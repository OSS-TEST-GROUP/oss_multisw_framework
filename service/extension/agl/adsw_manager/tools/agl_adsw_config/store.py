from __future__ import annotations

import glob
import os
import shutil
from datetime import datetime
from pathlib import Path
from typing import Optional

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq

from .model import ServiceEntry, ServiceList


_yaml = YAML()
_yaml.default_flow_style = False
_yaml.preserve_quotes = True
_yaml.width = 4096


def load(path: Path) -> tuple[dict, ServiceList]:
    """Load YAML and return (raw_dict, validated ServiceList)."""
    with open(path, "r") as f:
        raw = _yaml.load(f)
    if raw is None:
        raw = {"services": []}
    svc_list = ServiceList.model_validate(raw)
    return raw, svc_list


def save(path: Path, raw: dict, no_backup: bool = False, dry_run: bool = False) -> Optional[Path]:
    """Save YAML to disk. Returns backup path if created."""
    backup_path = None
    if not dry_run:
        if not no_backup and path.exists():
            backup_path = _make_backup(path)
        with open(path, "w") as f:
            _yaml.dump(raw, f)
    return backup_path


def _make_backup(path: Path) -> Path:
    ts = datetime.now().strftime("%Y%m%d-%H%M%S")
    backup_path = path.with_suffix(f".bak.{ts}")
    shutil.copy2(path, backup_path)
    return backup_path


def make_backup(path: Path) -> Path:
    return _make_backup(path)


def last_backup(path: Path) -> Optional[Path]:
    """Return the most recently created backup for the given YAML file."""
    # Backup names: <stem>.bak.<timestamp>  (suffix replaced by .bak.*)
    pattern = str(path.parent / (path.stem + ".bak.*"))
    matches = sorted(glob.glob(pattern))
    if not matches:
        return None
    return Path(matches[-1])


def raw_to_service_list(raw: dict) -> list[dict]:
    return list(raw.get("services", []))


def service_to_commented(entry: ServiceEntry) -> CommentedMap:
    cm = CommentedMap()
    cm["name"] = entry.name
    cm["service_type"] = entry.service_type.value
    cm["recovery_policy"] = entry.recovery_policy.value
    cm["is_enabled"] = entry.is_enabled
    cm["os_type"] = entry.os_type.value
    cm["init_state"] = entry.init_state.value
    cm["image"] = entry.image
    args = CommentedSeq(entry.extra_args)
    args.fa.set_block_style()
    cm["extra_args"] = args
    return cm


def add_service(raw: dict, entry: ServiceEntry) -> None:
    if "services" not in raw or raw["services"] is None:
        raw["services"] = CommentedSeq()
        raw["services"].fa.set_block_style()
    raw["services"].append(service_to_commented(entry))


def remove_service(raw: dict, name: str) -> bool:
    services = raw.get("services", [])
    for i, svc in enumerate(services):
        if svc["name"] == name:
            del services[i]
            return True
    return False


def update_service(raw: dict, name: str, updates: dict) -> bool:
    services = raw.get("services", [])
    for svc in services:
        if svc["name"] == name:
            for k, v in updates.items():
                svc[k] = v
            return True
    return False
