"""YAML load/save with ruamel.yaml (comment-preserving)."""
from __future__ import annotations

import os
import shutil
from datetime import datetime
from pathlib import Path
from typing import Optional, Tuple

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq

_DEFAULT_CONFIG_PATH = "./config/saved-service-list.yaml"


def get_default_config_path() -> str:
    env = os.environ.get("SAVED_REGISTRY_BACKUP")
    if env:
        return os.path.join(env, "saved-service-list.yaml")
    return _DEFAULT_CONFIG_PATH


def _make_yaml() -> YAML:
    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.default_flow_style = False
    yaml.width = 4096
    yaml.best_sequence_indent = 2
    yaml.best_map_flow_style = False
    return yaml


def load_yaml(path: str) -> CommentedMap:
    p = Path(path)
    if not p.exists():
        raise FileNotFoundError(f"Config file not found: {path}")
    yaml = _make_yaml()
    with open(path, "r", encoding="utf-8") as f:
        data = yaml.load(f)
    if data is None:
        data = CommentedMap({"version": 0.1, "specs": [{"containers": CommentedSeq()}]})
    # Ensure specs[0].containers exists
    if "specs" not in data or not data["specs"]:
        data["specs"] = CommentedSeq([CommentedMap({"containers": CommentedSeq()})])
    if "containers" not in data["specs"][0]:
        data["specs"][0]["containers"] = CommentedSeq()
    return data


def save_yaml(
    path: str,
    data: CommentedMap,
    backup: bool = True,
    dry_run: bool = False,
) -> Optional[str]:
    """Write data to path. Returns backup path if created."""
    if dry_run:
        return None

    backup_path: Optional[str] = None
    if backup and os.path.exists(path):
        ts = datetime.now().strftime("%Y%m%d-%H%M%S")
        backup_path = f"{path}.bak.{ts}"
        shutil.copy2(path, backup_path)

    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    yaml = _make_yaml()
    with open(path, "w", encoding="utf-8") as f:
        yaml.dump(data, f)
    return backup_path


def get_containers(data: CommentedMap) -> CommentedSeq:
    specs = data.get("specs") or []
    if not specs:
        return CommentedSeq()
    return specs[0].get("containers") or CommentedSeq()


def find_container(data: CommentedMap, name: str) -> Optional[Tuple[int, CommentedMap]]:
    for i, c in enumerate(get_containers(data)):
        if c.get("name") == name:
            return i, c
    return None


def find_service(container: CommentedMap, name: str) -> Optional[Tuple[int, CommentedMap]]:
    services = container.get("services") or []
    for i, s in enumerate(services):
        if s.get("name") == name:
            return i, s
    return None


def find_last_backup(path: str) -> Optional[str]:
    p = Path(path)
    backups = sorted(p.parent.glob(f"{p.name}.bak.*"))
    return str(backups[-1]) if backups else None
