"""Click CLI command definitions for adsw-config."""
from __future__ import annotations

import difflib
import json
import sys
from io import StringIO
from typing import Optional

import click
from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq

from . import formatter as fmt
from .store import (
    find_container,
    find_last_backup,
    find_service,
    get_containers,
    get_default_config_path,
    load_yaml,
    save_yaml,
)
from .validator import validate_data

# ─── Shared context helpers ───────────────────────────────────────────────────

_FILE_OPTION = click.option(
    "--file", "-f",
    default=None,
    show_default=True,
    help="Path to saved-service-list.yaml ($SAVED_REGISTRY_BACKUP/saved-service-list.yaml or ./config/saved-service-list.yaml)",
)
_DRY_RUN_OPTION = click.option("--dry-run", is_flag=True, default=False, help="Preview changes without writing.")
_NO_BACKUP_OPTION = click.option("--no-backup", is_flag=True, default=False, help="Skip backup when saving.")
_OUTPUT_OPTION = click.option(
    "--output", "-o",
    type=click.Choice(["table", "json", "yaml"], case_sensitive=False),
    default="table",
    show_default=True,
    help="Output format.",
)


def _resolve_file(file: Optional[str]) -> str:
    return file or get_default_config_path()


def _load(ctx: click.Context) -> CommentedMap:
    path = ctx.obj["file"]
    try:
        return load_yaml(path)
    except FileNotFoundError as e:
        fmt.print_error(str(e))
        sys.exit(1)


def _save(ctx: click.Context, data: CommentedMap, *, action: str = "Saved") -> None:
    path = ctx.obj["file"]
    dry_run = ctx.obj["dry_run"]
    do_backup = not ctx.obj["no_backup"]

    if dry_run:
        fmt.print_info("[dry-run] No changes written.")
        return

    backup = save_yaml(path, data, backup=do_backup, dry_run=False)
    fmt.print_success(f"{action}: {path}")
    if backup:
        fmt.print_info(f"Backup:  {backup}")


def _data_to_yaml_str(data: CommentedMap) -> str:
    buf = StringIO()
    yaml = YAML()
    yaml.default_flow_style = False
    yaml.dump(data, buf)
    return buf.getvalue()


# ─── Root group ───────────────────────────────────────────────────────────────

@click.group()
@_FILE_OPTION
@_DRY_RUN_OPTION
@_NO_BACKUP_OPTION
@_OUTPUT_OPTION
@click.pass_context
def cli(ctx: click.Context, file: Optional[str], dry_run: bool, no_backup: bool, output: str) -> None:
    """adsw-config – manage saved-service-list.yaml from the command line."""
    ctx.ensure_object(dict)
    ctx.obj["file"] = _resolve_file(file)
    ctx.obj["dry_run"] = dry_run
    ctx.obj["no_backup"] = no_backup
    ctx.obj["output"] = output


# ─── show ─────────────────────────────────────────────────────────────────────

@cli.command("show")
@click.pass_context
def cmd_show(ctx: click.Context) -> None:
    """Display the full config as a tree."""
    data = _load(ctx)
    output = ctx.obj["output"]
    if output == "json":
        fmt.console.print_json(json.dumps(data, default=str))
    elif output == "yaml":
        fmt.console.print(_data_to_yaml_str(data))
    else:
        fmt.show_tree(data)


# ─── list ─────────────────────────────────────────────────────────────────────

@cli.group("list")
@click.pass_context
def cmd_list(ctx: click.Context) -> None:
    """List containers or services."""


@cmd_list.command("containers")
@click.pass_context
def list_containers(ctx: click.Context) -> None:
    """List all containers."""
    data = _load(ctx)
    fmt.list_containers_table(data, output=ctx.obj["output"])


@cmd_list.command("services")
@click.option("--container", "-c", default=None, help="Filter by container name.")
@click.pass_context
def list_services(ctx: click.Context, container: Optional[str]) -> None:
    """List services, optionally filtered by container."""
    data = _load(ctx)
    fmt.list_services_table(data, container_filter=container, output=ctx.obj["output"])


# ─── container ────────────────────────────────────────────────────────────────

@cli.group("container")
@click.pass_context
def cmd_container(ctx: click.Context) -> None:
    """Manage containers."""


@cmd_container.command("add")
@click.argument("name")
@click.option("--image", required=True, help="Container image (e.g. my-app:latest).")
@click.option(
    "--type", "ctype",
    required=True,
    type=click.Choice(["docker", "docker-compose", "k3s"], case_sensitive=False),
    help="Container type.",
)
@click.option("--deploy-filepath", default=None, help="Compose/k3s manifest path.")
@click.option("--namespace", default=None, help="k3s namespace.")
@click.pass_context
def container_add(
    ctx: click.Context,
    name: str,
    image: str,
    ctype: str,
    deploy_filepath: Optional[str],
    namespace: Optional[str],
) -> None:
    """Add a new container."""
    data = _load(ctx)

    if find_container(data, name) is not None:
        fmt.print_error(f"Container '{name}' already exists.")
        sys.exit(1)

    # Validation
    if ctype in ("docker-compose", "k3s") and not deploy_filepath:
        fmt.print_error(f"--deploy-filepath is required for container-type '{ctype}'.")
        sys.exit(1)
    if ctype == "k3s" and not namespace:
        fmt.print_error("--namespace is required for container-type 'k3s'.")
        sys.exit(1)

    entry = CommentedMap({"name": name, "image": image, "container-type": ctype})
    if deploy_filepath:
        entry["deploy-filepath"] = deploy_filepath
    if namespace:
        entry["namespace"] = namespace
    entry["services"] = CommentedSeq()

    containers = get_containers(data)
    containers.append(entry)

    if ctx.obj["dry_run"]:
        fmt.print_info(f"[dry-run] Would add container '{name}'.")
        return
    _save(ctx, data, action=f"Container '{name}' added")


@cmd_container.command("remove")
@click.argument("name")
@click.option("--force", is_flag=True, default=False, help="Remove even if services exist.")
@click.pass_context
def container_remove(ctx: click.Context, name: str, force: bool) -> None:
    """Remove a container by name."""
    data = _load(ctx)

    result = find_container(data, name)
    if result is None:
        fmt.print_error(f"Container '{name}' not found.")
        sys.exit(1)

    idx, container = result
    services = container.get("services") or []
    if services and not force:
        fmt.print_error(
            f"Container '{name}' has {len(services)} service(s). Use --force to remove."
        )
        sys.exit(1)

    containers = get_containers(data)
    del containers[idx]

    if ctx.obj["dry_run"]:
        fmt.print_info(f"[dry-run] Would remove container '{name}'.")
        return
    _save(ctx, data, action=f"Container '{name}' removed")


@cmd_container.command("set")
@click.argument("name")
@click.option("--image", default=None)
@click.option("--type", "ctype", default=None, type=click.Choice(["docker", "docker-compose", "k3s"]))
@click.option("--deploy-filepath", default=None)
@click.option("--namespace", default=None)
@click.pass_context
def container_set(
    ctx: click.Context,
    name: str,
    image: Optional[str],
    ctype: Optional[str],
    deploy_filepath: Optional[str],
    namespace: Optional[str],
) -> None:
    """Update container fields."""
    data = _load(ctx)

    result = find_container(data, name)
    if result is None:
        fmt.print_error(f"Container '{name}' not found.")
        sys.exit(1)

    _, container = result
    if image is not None:
        container["image"] = image
    if ctype is not None:
        container["container-type"] = ctype
    if deploy_filepath is not None:
        container["deploy-filepath"] = deploy_filepath
    if namespace is not None:
        container["namespace"] = namespace

    # Re-validate
    new_ct = container.get("container-type", "")
    if new_ct in ("docker-compose", "k3s") and not container.get("deploy-filepath"):
        fmt.print_error(f"deploy-filepath is required for container-type '{new_ct}'.")
        sys.exit(1)
    if new_ct == "k3s" and not container.get("namespace"):
        fmt.print_error("namespace is required for container-type 'k3s'.")
        sys.exit(1)

    if ctx.obj["dry_run"]:
        fmt.print_info(f"[dry-run] Would update container '{name}'.")
        return
    _save(ctx, data, action=f"Container '{name}' updated")


# ─── service ──────────────────────────────────────────────────────────────────

@cli.group("service")
@click.pass_context
def cmd_service(ctx: click.Context) -> None:
    """Manage services."""


@cmd_service.command("add")
@click.argument("container")
@click.argument("name")
@click.option("--package", required=True, help="ROS2 package name.")
@click.option("--launchfile", default="", help="Launch file name.")
@click.option(
    "--type", "stype",
    default="NORMAL",
    type=click.Choice(["NORMAL", "ONESHOT"], case_sensitive=True),
    help="Service type.",
)
@click.option(
    "--recovery",
    default="ALWAYS",
    type=click.Choice(["ALWAYS", "ON_FAILURE", "ON_SUCCESS", "ON_NOT_RESPONDING", "MANUAL"], case_sensitive=True),
)
@click.option("--enabled", default="true", type=click.Choice(["true", "false"]))
@click.option("--max", "max_count", default="0")
@click.pass_context
def service_add(
    ctx: click.Context,
    container: str,
    name: str,
    package: str,
    launchfile: str,
    stype: str,
    recovery: str,
    enabled: str,
    max_count: str,
) -> None:
    """Add a service to a container."""
    data = _load(ctx)

    result = find_container(data, container)
    if result is None:
        fmt.print_error(f"Container '{container}' not found.")
        sys.exit(1)

    _, cont = result
    if "services" not in cont or cont["services"] is None:
        cont["services"] = CommentedSeq()

    if find_service(cont, name) is not None:
        fmt.print_error(f"Service '{name}' already exists in container '{container}'.")
        sys.exit(1)

    entry = CommentedMap({
        "package": package,
        "name": name,
        "launchfile": launchfile,
        "type": stype,
        "recovery": recovery,
        "enabled": enabled,
        "max": max_count,
    })
    cont["services"].append(entry)

    if ctx.obj["dry_run"]:
        fmt.print_info(f"[dry-run] Would add service '{name}' to container '{container}'.")
        return
    _save(ctx, data, action=f"Service '{container}/{name}' added")


@cmd_service.command("remove")
@click.argument("container")
@click.argument("name")
@click.pass_context
def service_remove(ctx: click.Context, container: str, name: str) -> None:
    """Remove a service from a container."""
    data = _load(ctx)

    result = find_container(data, container)
    if result is None:
        fmt.print_error(f"Container '{container}' not found.")
        sys.exit(1)

    _, cont = result
    svc_result = find_service(cont, name)
    if svc_result is None:
        fmt.print_error(f"Service '{name}' not found in container '{container}'.")
        sys.exit(1)

    svc_idx, _ = svc_result
    del cont["services"][svc_idx]

    if ctx.obj["dry_run"]:
        fmt.print_info(f"[dry-run] Would remove service '{name}' from container '{container}'.")
        return
    _save(ctx, data, action=f"Service '{container}/{name}' removed")


@cmd_service.command("set")
@click.argument("container")
@click.argument("name")
@click.option("--package", default=None)
@click.option("--launchfile", default=None)
@click.option("--type", "stype", default=None, type=click.Choice(["NORMAL", "ONESHOT"]))
@click.option(
    "--recovery", default=None,
    type=click.Choice(["ALWAYS", "ON_FAILURE", "ON_SUCCESS", "ON_NOT_RESPONDING", "MANUAL"]),
)
@click.option("--enabled", default=None, type=click.Choice(["true", "false"]))
@click.option("--max", "max_count", default=None)
@click.pass_context
def service_set(
    ctx: click.Context,
    container: str,
    name: str,
    package: Optional[str],
    launchfile: Optional[str],
    stype: Optional[str],
    recovery: Optional[str],
    enabled: Optional[str],
    max_count: Optional[str],
) -> None:
    """Update service fields."""
    data = _load(ctx)

    result = find_container(data, container)
    if result is None:
        fmt.print_error(f"Container '{container}' not found.")
        sys.exit(1)

    _, cont = result
    svc_result = find_service(cont, name)
    if svc_result is None:
        fmt.print_error(f"Service '{name}' not found in container '{container}'.")
        sys.exit(1)

    _, svc = svc_result
    if package is not None:
        svc["package"] = package
    if launchfile is not None:
        svc["launchfile"] = launchfile
    if stype is not None:
        svc["type"] = stype
    if recovery is not None:
        svc["recovery"] = recovery
    if enabled is not None:
        svc["enabled"] = enabled
    if max_count is not None:
        svc["max"] = max_count

    if ctx.obj["dry_run"]:
        fmt.print_info(f"[dry-run] Would update service '{container}/{name}'.")
        return
    _save(ctx, data, action=f"Service '{container}/{name}' updated")


def _set_enabled(ctx: click.Context, container: str, name: str, value: str) -> None:
    data = _load(ctx)

    result = find_container(data, container)
    if result is None:
        fmt.print_error(f"Container '{container}' not found.")
        sys.exit(1)

    _, cont = result
    svc_result = find_service(cont, name)
    if svc_result is None:
        fmt.print_error(f"Service '{name}' not found in container '{container}'.")
        sys.exit(1)

    _, svc = svc_result
    svc["enabled"] = value

    if ctx.obj["dry_run"]:
        word = "enable" if value == "true" else "disable"
        fmt.print_info(f"[dry-run] Would {word} service '{container}/{name}'.")
        return
    action = "enabled" if value == "true" else "disabled"
    _save(ctx, data, action=f"Service '{container}/{name}' {action}")


@cmd_service.command("enable")
@click.argument("container")
@click.argument("name")
@click.pass_context
def service_enable(ctx: click.Context, container: str, name: str) -> None:
    """Enable a service."""
    _set_enabled(ctx, container, name, "true")


@cmd_service.command("disable")
@click.argument("container")
@click.argument("name")
@click.pass_context
def service_disable(ctx: click.Context, container: str, name: str) -> None:
    """Disable a service."""
    _set_enabled(ctx, container, name, "false")


# ─── validate ─────────────────────────────────────────────────────────────────

@cli.command("validate")
@click.pass_context
def cmd_validate(ctx: click.Context) -> None:
    """Validate the YAML structure and values."""
    data = _load(ctx)
    errors = validate_data(data)
    if errors:
        fmt.print_error(f"Validation failed with {len(errors)} error(s):")
        for e in errors:
            fmt.console.print(f"  [red]•[/red] {e}")
        sys.exit(1)
    else:
        fmt.print_success("Validation passed.")


# ─── backup ───────────────────────────────────────────────────────────────────

@cli.command("backup")
@click.pass_context
def cmd_backup(ctx: click.Context) -> None:
    """Create a timestamped backup of the config file."""
    import shutil
    from datetime import datetime
    from pathlib import Path

    path = ctx.obj["file"]
    if not Path(path).exists():
        fmt.print_error(f"File not found: {path}")
        sys.exit(1)

    if ctx.obj["dry_run"]:
        from datetime import datetime
        ts = datetime.now().strftime("%Y%m%d-%H%M%S")
        fmt.print_info(f"[dry-run] Would create backup: {path}.bak.{ts}")
        return

    from datetime import datetime
    ts = datetime.now().strftime("%Y%m%d-%H%M%S")
    backup_path = f"{path}.bak.{ts}"
    shutil.copy2(path, backup_path)
    fmt.print_success(f"Backup created: {backup_path}")


# ─── diff ─────────────────────────────────────────────────────────────────────

@cli.command("diff")
@click.pass_context
def cmd_diff(ctx: click.Context) -> None:
    """Show diff between the last backup and the current file."""
    from pathlib import Path

    path = ctx.obj["file"]
    if not Path(path).exists():
        fmt.print_error(f"File not found: {path}")
        sys.exit(1)

    backup = find_last_backup(path)
    if backup is None:
        fmt.print_warning("No backup found. Run 'adsw-config backup' first.")
        return

    with open(backup, "r", encoding="utf-8") as f:
        old_lines = f.readlines()
    with open(path, "r", encoding="utf-8") as f:
        new_lines = f.readlines()

    diff = list(difflib.unified_diff(
        old_lines, new_lines,
        fromfile=f"backup ({Path(backup).name})",
        tofile=f"current ({Path(path).name})",
    ))

    if not diff:
        fmt.print_info("No differences found.")
        return

    for line in diff:
        line = line.rstrip("\n")
        if line.startswith("+++") or line.startswith("---"):
            fmt.console.print(f"[bold]{line}[/bold]")
        elif line.startswith("+"):
            fmt.console.print(f"[green]{line}[/green]")
        elif line.startswith("-"):
            fmt.console.print(f"[red]{line}[/red]")
        elif line.startswith("@@"):
            fmt.console.print(f"[cyan]{line}[/cyan]")
        else:
            fmt.console.print(line)
