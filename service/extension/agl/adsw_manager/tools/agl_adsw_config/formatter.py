from __future__ import annotations

import difflib
import json
from io import StringIO
from pathlib import Path
from typing import List

from rich.console import Console
from rich.table import Table
from rich.tree import Tree
from rich import box
from ruamel.yaml import YAML

from .model import ServiceEntry, ServiceList

console = Console()
_yaml_out = YAML()
_yaml_out.default_flow_style = False


# ── helpers ──────────────────────────────────────────────────────────────────

def _enabled_icon(is_enabled: bool) -> str:
    return "[green]●[/green]" if is_enabled else "[dim]○[/dim]"


# ── show (tree) ───────────────────────────────────────────────────────────────

def show_tree(svc_list: ServiceList) -> None:
    tree = Tree("[bold cyan]agl-adsw-config[/bold cyan]  [dim]v0.1[/dim]")
    for svc in svc_list.services:
        icon = _enabled_icon(svc.is_enabled)
        label = (
            f"{icon} [bold]{svc.name}[/bold]  "
            f"[yellow]{svc.os_type.value}[/yellow]  "
            f"docker=[cyan]{svc.image}[/cyan]  "
            f"recovery=[magenta]{svc.recovery_policy.value}[/magenta]"
        )
        tree.add(label)
    console.print(tree)


# ── list (table) ──────────────────────────────────────────────────────────────

def show_table(svc_list: ServiceList) -> None:
    table = Table(box=box.ROUNDED, show_header=True, header_style="bold cyan")
    table.add_column("", width=2)
    table.add_column("Name", style="bold")
    table.add_column("Type")
    table.add_column("Recovery")
    table.add_column("OS")
    table.add_column("Init State")
    table.add_column("Image")
    for svc in svc_list.services:
        icon = "●" if svc.is_enabled else "○"
        icon_style = "green" if svc.is_enabled else "dim"
        table.add_row(
            f"[{icon_style}]{icon}[/{icon_style}]",
            svc.name,
            svc.service_type.value,
            svc.recovery_policy.value,
            svc.os_type.value,
            svc.init_state.value,
            svc.image,
        )
    console.print(table)


# ── get (detail) ──────────────────────────────────────────────────────────────

def show_service_detail(svc: ServiceEntry, output_fmt: str = "table") -> None:
    if output_fmt == "json":
        console.print_json(json.dumps(svc.model_dump(), indent=2))
        return
    if output_fmt == "yaml":
        buf = StringIO()
        _yaml_out.dump(svc.model_dump(), buf)
        console.print(buf.getvalue())
        return
    # table
    table = Table(box=box.SIMPLE, show_header=False)
    table.add_column("Field", style="bold cyan", min_width=20)
    table.add_column("Value")
    icon = "[green]●[/green]" if svc.is_enabled else "[dim]○[/dim]"
    rows = [
        ("name", svc.name),
        ("enabled", icon),
        ("service_type", svc.service_type.value),
        ("recovery_policy", svc.recovery_policy.value),
        ("os_type", svc.os_type.value),
        ("init_state", svc.init_state.value),
        ("image", svc.image),
        ("extra_args", "\n".join(svc.extra_args) if svc.extra_args else "[dim](none)[/dim]"),
    ]
    for field, val in rows:
        table.add_row(field, val)
    console.print(table)


# ── args list ─────────────────────────────────────────────────────────────────

def show_args(svc: ServiceEntry) -> None:
    if not svc.extra_args:
        console.print(f"[dim]No extra_args for '{svc.name}'[/dim]")
        return
    table = Table(box=box.SIMPLE, show_header=True, header_style="bold cyan")
    table.add_column("#", style="dim", width=4)
    table.add_column("Argument")
    for i, arg in enumerate(svc.extra_args, 1):
        table.add_row(str(i), arg)
    console.print(table)


# ── validate results ──────────────────────────────────────────────────────────

def show_validation(ok: bool, errors: List[str], path: Path) -> None:
    if ok:
        console.print(f"[green]✔[/green] [bold]{path}[/bold] is valid.")
    else:
        console.print(f"[red]✘[/red] [bold]{path}[/bold] has {len(errors)} error(s):")
        for err in errors:
            console.print(f"  [red]•[/red] {err}")


# ── diff ──────────────────────────────────────────────────────────────────────

def show_diff(old_path: Path, new_path: Path) -> None:
    old_lines = old_path.read_text().splitlines(keepends=True)
    new_lines = new_path.read_text().splitlines(keepends=True)
    diff = list(difflib.unified_diff(
        old_lines,
        new_lines,
        fromfile=str(old_path),
        tofile=str(new_path),
    ))
    if not diff:
        console.print("[dim]No differences found.[/dim]")
        return
    for line in diff:
        line = line.rstrip("\n")
        if line.startswith("+++") or line.startswith("---"):
            console.print(f"[bold]{line}[/bold]")
        elif line.startswith("+"):
            console.print(f"[green]{line}[/green]")
        elif line.startswith("-"):
            console.print(f"[red]{line}[/red]")
        elif line.startswith("@@"):
            console.print(f"[cyan]{line}[/cyan]")
        else:
            console.print(line)


# ── dry-run preview ───────────────────────────────────────────────────────────

def show_dry_run(raw: dict) -> None:
    buf = StringIO()
    _yaml_out.dump(raw, buf)
    console.print("[bold yellow][DRY-RUN] Would write:[/bold yellow]")
    console.print(buf.getvalue())


# ── generic output (list/json/yaml) ──────────────────────────────────────────

def show_list_output(svc_list: ServiceList, output_fmt: str) -> None:
    if output_fmt == "json":
        console.print_json(json.dumps(svc_list.model_dump(), indent=2))
    elif output_fmt == "yaml":
        buf = StringIO()
        _yaml_out.dump(svc_list.model_dump(), buf)
        console.print(buf.getvalue())
    else:
        show_table(svc_list)
