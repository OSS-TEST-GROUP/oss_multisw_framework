from __future__ import annotations

import sys
from pathlib import Path
from typing import Optional

import click
from rich.console import Console

from . import store, formatter, validator
from .model import (
    ServiceEntry, ServiceList,
    ServiceType, RecoveryPolicy, OsType, InitState,
)

console = Console(stderr=True)

DEFAULT_FILE = Path("config/agl-service-list.yaml")


# ── helpers ───────────────────────────────────────────────────────────────────

def _abort(msg: str) -> None:
    console.print(f"[red]Error:[/red] {msg}")
    sys.exit(1)


def _resolve_file(ctx_obj: dict) -> Path:
    return Path(ctx_obj["file"])


def _load(ctx_obj: dict):
    path = _resolve_file(ctx_obj)
    if not path.exists():
        _abort(f"File not found: {path}")
    try:
        return path, *store.load(path)
    except Exception as e:
        _abort(f"Failed to load {path}: {e}")


def _save(ctx_obj: dict, path: Path, raw: dict) -> None:
    dry_run = ctx_obj["dry_run"]
    no_backup = ctx_obj["no_backup"]
    if dry_run:
        formatter.show_dry_run(raw)
        return
    backup = store.save(path, raw, no_backup=no_backup)
    if backup:
        console.print(f"[dim]Backup: {backup}[/dim]")


# ── root group ────────────────────────────────────────────────────────────────

@click.group()
@click.option("--file", "-f", default=str(DEFAULT_FILE), show_default=True,
              help="Target YAML file")
@click.option("--dry-run", is_flag=True, default=False,
              help="Preview changes without writing")
@click.option("--no-backup", is_flag=True, default=False,
              help="Skip backup before saving")
@click.option("--output", "-o",
              type=click.Choice(["table", "json", "yaml"], case_sensitive=False),
              default="table", show_default=True,
              help="Output format")
@click.pass_context
def cli(ctx: click.Context, file: str, dry_run: bool, no_backup: bool, output: str) -> None:
    """AGL ADSW config manager."""
    ctx.ensure_object(dict)
    ctx.obj["file"] = file
    ctx.obj["dry_run"] = dry_run
    ctx.obj["no_backup"] = no_backup
    ctx.obj["output"] = output


# ── show ──────────────────────────────────────────────────────────────────────

@cli.command()
@click.pass_context
def show(ctx: click.Context) -> None:
    """Print service tree."""
    path, raw, svc_list = _load(ctx.obj)
    formatter.show_tree(svc_list)


# ── list ──────────────────────────────────────────────────────────────────────

@cli.command(name="list")
@click.pass_context
def list_cmd(ctx: click.Context) -> None:
    """List services as a table."""
    path, raw, svc_list = _load(ctx.obj)
    formatter.show_list_output(svc_list, ctx.obj["output"])


# ── get ───────────────────────────────────────────────────────────────────────

@cli.command()
@click.argument("name")
@click.pass_context
def get(ctx: click.Context, name: str) -> None:
    """Show details for a specific service."""
    path, raw, svc_list = _load(ctx.obj)
    svc = next((s for s in svc_list.services if s.name == name), None)
    if svc is None:
        _abort(f"Service '{name}' not found")
    formatter.show_service_detail(svc, ctx.obj["output"])


# ── add ───────────────────────────────────────────────────────────────────────

@cli.command()
@click.argument("name")
@click.option("--image", required=True, help="Docker image")
@click.option("--type", "service_type", default="NORMAL",
              type=click.Choice([e.value for e in ServiceType], case_sensitive=False),
              show_default=True)
@click.option("--recovery", "recovery_policy", default="R_DEFAULT",
              type=click.Choice([e.value for e in RecoveryPolicy], case_sensitive=False),
              show_default=True)
@click.option("--enabled", "is_enabled", default="true",
              type=click.Choice(["true", "false"], case_sensitive=False),
              show_default=True)
@click.option("--os-type", "os_type", default="AGL",
              type=click.Choice([e.value for e in OsType], case_sensitive=False),
              show_default=True)
@click.option("--init-state", "init_state", default="NOT_RUNNING",
              type=click.Choice([e.value for e in InitState], case_sensitive=False),
              show_default=True)
@click.option("--extra-args", "extra_args", multiple=True,
              help="extra_args entries (repeatable)")
@click.pass_context
def add(ctx: click.Context, name: str, image: str, service_type: str,
        recovery_policy: str, is_enabled: str, os_type: str,
        init_state: str, extra_args: tuple) -> None:
    """Add a new service."""
    path, raw, svc_list = _load(ctx.obj)
    if any(s.name == name for s in svc_list.services):
        _abort(f"Service '{name}' already exists")
    entry = ServiceEntry(
        name=name,
        image=image,
        service_type=ServiceType(service_type.upper()),
        recovery_policy=RecoveryPolicy(recovery_policy.upper()),
        is_enabled=(is_enabled.lower() == "true"),
        os_type=OsType(os_type.upper()),
        init_state=InitState(init_state.upper()),
        extra_args=list(extra_args),
    )
    store.add_service(raw, entry)
    _save(ctx.obj, path, raw)
    if not ctx.obj["dry_run"]:
        console.print(f"[green]Added[/green] service '{name}'")


# ── remove ────────────────────────────────────────────────────────────────────

@cli.command()
@click.argument("name")
@click.pass_context
def remove(ctx: click.Context, name: str) -> None:
    """Remove a service."""
    path, raw, svc_list = _load(ctx.obj)
    if not any(s.name == name for s in svc_list.services):
        _abort(f"Service '{name}' not found")
    store.remove_service(raw, name)
    _save(ctx.obj, path, raw)
    if not ctx.obj["dry_run"]:
        console.print(f"[red]Removed[/red] service '{name}'")


# ── set ───────────────────────────────────────────────────────────────────────

@cli.command(name="set")
@click.argument("name")
@click.option("--image", default=None)
@click.option("--type", "service_type", default=None,
              type=click.Choice([e.value for e in ServiceType], case_sensitive=False))
@click.option("--recovery", "recovery_policy", default=None,
              type=click.Choice([e.value for e in RecoveryPolicy], case_sensitive=False))
@click.option("--enabled", "is_enabled", default=None,
              type=click.Choice(["true", "false"], case_sensitive=False))
@click.option("--os-type", "os_type", default=None,
              type=click.Choice([e.value for e in OsType], case_sensitive=False))
@click.option("--init-state", "init_state", default=None,
              type=click.Choice([e.value for e in InitState], case_sensitive=False))
@click.pass_context
def set_cmd(ctx: click.Context, name: str, image: Optional[str],
            service_type: Optional[str], recovery_policy: Optional[str],
            is_enabled: Optional[str], os_type: Optional[str],
            init_state: Optional[str]) -> None:
    """Update service attributes."""
    path, raw, svc_list = _load(ctx.obj)
    if not any(s.name == name for s in svc_list.services):
        _abort(f"Service '{name}' not found")
    updates: dict = {}
    if image is not None:
        updates["image"] = image
    if service_type is not None:
        updates["service_type"] = service_type.upper()
    if recovery_policy is not None:
        updates["recovery_policy"] = recovery_policy.upper()
    if is_enabled is not None:
        updates["is_enabled"] = (is_enabled.lower() == "true")
    if os_type is not None:
        updates["os_type"] = os_type.upper()
    if init_state is not None:
        updates["init_state"] = init_state.upper()
    if not updates:
        console.print("[yellow]Nothing to update.[/yellow]")
        return
    store.update_service(raw, name, updates)
    _save(ctx.obj, path, raw)
    if not ctx.obj["dry_run"]:
        console.print(f"[green]Updated[/green] service '{name}'")


# ── enable / disable ──────────────────────────────────────────────────────────

@cli.command()
@click.argument("name")
@click.pass_context
def enable(ctx: click.Context, name: str) -> None:
    """Enable a service (is_enabled=true)."""
    path, raw, svc_list = _load(ctx.obj)
    if not any(s.name == name for s in svc_list.services):
        _abort(f"Service '{name}' not found")
    store.update_service(raw, name, {"is_enabled": True})
    _save(ctx.obj, path, raw)
    if not ctx.obj["dry_run"]:
        console.print(f"[green]Enabled[/green] '{name}'")


@cli.command()
@click.argument("name")
@click.pass_context
def disable(ctx: click.Context, name: str) -> None:
    """Disable a service (is_enabled=false)."""
    path, raw, svc_list = _load(ctx.obj)
    if not any(s.name == name for s in svc_list.services):
        _abort(f"Service '{name}' not found")
    store.update_service(raw, name, {"is_enabled": False})
    _save(ctx.obj, path, raw)
    if not ctx.obj["dry_run"]:
        console.print(f"[yellow]Disabled[/yellow] '{name}'")


# ── args ──────────────────────────────────────────────────────────────────────

@cli.command(name="args")
@click.argument("name")
@click.option("--add", "add_arg", default=None, metavar="ARG",
              help="Add an entry to extra_args")
@click.option("--remove", "remove_arg", default=None, metavar="ARG",
              help="Remove an entry from extra_args")
@click.option("--list", "list_args", is_flag=True, default=False,
              help="List extra_args")
@click.pass_context
def args_cmd(ctx: click.Context, name: str, add_arg: Optional[str],
             remove_arg: Optional[str], list_args: bool) -> None:
    """Manage extra_args for a service."""
    path, raw, svc_list = _load(ctx.obj)
    svc = next((s for s in svc_list.services if s.name == name), None)
    if svc is None:
        _abort(f"Service '{name}' not found")

    if list_args:
        formatter.show_args(svc)
        return

    if add_arg is None and remove_arg is None:
        _abort("Specify --add, --remove, or --list")

    # work on raw to preserve YAML structure
    raw_services = raw.get("services", [])
    raw_svc = next((s for s in raw_services if s["name"] == name), None)
    if raw_svc is None:
        _abort(f"Service '{name}' not found in raw YAML")

    if "extra_args" not in raw_svc or raw_svc["extra_args"] is None:
        from ruamel.yaml.comments import CommentedSeq
        raw_svc["extra_args"] = CommentedSeq()
        raw_svc["extra_args"].fa.set_block_style()

    if add_arg is not None:
        if add_arg in list(raw_svc["extra_args"]):
            console.print(f"[yellow]'{add_arg}' already in extra_args[/yellow]")
        else:
            raw_svc["extra_args"].append(add_arg)
            _save(ctx.obj, path, raw)
            if not ctx.obj["dry_run"]:
                console.print(f"[green]Added[/green] '{add_arg}' to extra_args of '{name}'")

    if remove_arg is not None:
        args_list = list(raw_svc["extra_args"])
        if remove_arg not in args_list:
            _abort(f"'{remove_arg}' not found in extra_args of '{name}'")
        args_list.remove(remove_arg)
        from ruamel.yaml.comments import CommentedSeq
        new_seq = CommentedSeq(args_list)
        new_seq.fa.set_block_style()
        raw_svc["extra_args"] = new_seq
        _save(ctx.obj, path, raw)
        if not ctx.obj["dry_run"]:
            console.print(f"[red]Removed[/red] '{remove_arg}' from extra_args of '{name}'")


# ── validate ──────────────────────────────────────────────────────────────────

@cli.command()
@click.pass_context
def validate(ctx: click.Context) -> None:
    """Validate the YAML file."""
    path = _resolve_file(ctx.obj)
    if not path.exists():
        _abort(f"File not found: {path}")
    ok, errors = validator.validate_file(path)
    formatter.show_validation(ok, errors, path)
    if not ok:
        sys.exit(1)


# ── backup ────────────────────────────────────────────────────────────────────

@cli.command()
@click.pass_context
def backup(ctx: click.Context) -> None:
    """Create a timestamped backup of the YAML file."""
    path = _resolve_file(ctx.obj)
    if not path.exists():
        _abort(f"File not found: {path}")
    bak = store.make_backup(path)
    console.print(f"[green]Backup created:[/green] {bak}")


# ── diff ──────────────────────────────────────────────────────────────────────

@cli.command()
@click.pass_context
def diff(ctx: click.Context) -> None:
    """Show diff between last backup and current file."""
    path = _resolve_file(ctx.obj)
    if not path.exists():
        _abort(f"File not found: {path}")
    last_bak = store.last_backup(path)
    if last_bak is None:
        _abort(f"No backup found for {path}")
    formatter.show_diff(last_bak, path)
