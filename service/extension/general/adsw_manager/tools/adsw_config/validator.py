"""Validation logic for adsw-config YAML data."""
from __future__ import annotations

from typing import List

from ruamel.yaml.comments import CommentedMap

VALID_CONTAINER_TYPES = {"docker", "docker-compose", "k3s"}
VALID_SERVICE_TYPES = {"NORMAL", "ONESHOT"}
VALID_RECOVERY_TYPES = {"ALWAYS", "ON_FAILURE", "ON_SUCCESS", "ON_NOT_RESPONDING", "MANUAL"}


def validate_data(data: CommentedMap) -> List[str]:
    """Validate the full config. Returns a list of error strings (empty = OK)."""
    errors: List[str] = []
    specs = data.get("specs") or []
    for spec in specs:
        containers = spec.get("containers") or []
        for container in containers:
            errors.extend(_validate_container(container))
    return errors


def _validate_container(c: CommentedMap) -> List[str]:
    errors: List[str] = []
    name = c.get("name", "<unnamed>")
    ct = c.get("container-type", "")

    if not c.get("name"):
        errors.append("Container missing required field 'name'")
    if not c.get("image"):
        errors.append(f"Container '{name}': missing required field 'image'")

    if ct not in VALID_CONTAINER_TYPES:
        errors.append(
            f"Container '{name}': invalid container-type '{ct}'"
            f" (allowed: {', '.join(sorted(VALID_CONTAINER_TYPES))})"
        )
    if ct in ("docker-compose", "k3s") and not c.get("deploy-filepath"):
        errors.append(f"Container '{name}': deploy-filepath is required for container-type '{ct}'")
    if ct == "k3s" and not c.get("namespace"):
        errors.append(f"Container '{name}': namespace is required for container-type 'k3s'")

    seen: set = set()
    for svc in c.get("services") or []:
        sn = svc.get("name", "")
        if sn in seen:
            errors.append(f"Container '{name}': duplicate service name '{sn}'")
        seen.add(sn)
        errors.extend(_validate_service(name, svc))

    return errors


def _validate_service(container_name: str, s: CommentedMap) -> List[str]:
    errors: List[str] = []
    name = s.get("name", "<unnamed>")
    prefix = f"Service '{container_name}/{name}'"

    if not s.get("name"):
        errors.append(f"Service in container '{container_name}': missing required field 'name'")
    if not s.get("package"):
        errors.append(f"{prefix}: missing required field 'package'")

    svc_type = s.get("type", "")
    if svc_type not in VALID_SERVICE_TYPES:
        errors.append(
            f"{prefix}: invalid type '{svc_type}'"
            f" (allowed: {', '.join(sorted(VALID_SERVICE_TYPES))})"
        )

    recovery = s.get("recovery", "")
    if recovery not in VALID_RECOVERY_TYPES:
        errors.append(
            f"{prefix}: invalid recovery '{recovery}'"
            f" (allowed: {', '.join(sorted(VALID_RECOVERY_TYPES))})"
        )

    enabled = s.get("enabled", "")
    if enabled not in ("true", "false"):
        errors.append(f"{prefix}: enabled must be 'true' or 'false', got '{enabled}'")

    return errors
