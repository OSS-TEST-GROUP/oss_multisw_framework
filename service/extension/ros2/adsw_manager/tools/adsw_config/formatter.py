"""Rich-based output formatting for adsw-config."""
from __future__ import annotations

import json
from io import StringIO
from typing import Optional

from rich import box
from rich.console import Console
from rich.table import Table
from rich.tree import Tree
from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap

console = Console()


# ─── Tree ─────────────────────────────────────────────────────────────────────

def show_tree(data: CommentedMap) -> None:
    version = data.get("version", "?")
    root = Tree(f"[bold blue]adsw-config[/bold blue]  [dim]v{version}[/dim]")

    specs = data.get("specs") or []
    for spec in specs:
        for c in spec.get("containers") or []:
            ct = c.get("container-type", "?")
            img = c.get("image", "?")
            c_node = root.add(
                f"[bold green]{c.get('name', '?')}[/bold green]"
                f"  [dim cyan]{ct}[/dim cyan]"
            )
            c_node.add(f"[dim]image: {img}[/dim]")
            if c.get("deploy-filepath"):
                c_node.add(f"[dim]deploy-filepath: {c['deploy-filepath']}[/dim]")
            if c.get("namespace"):
                c_node.add(f"[dim]namespace: {c['namespace']}[/dim]")

            services = c.get("services") or []
            if services:
                svc_branch = c_node.add("[yellow]services[/yellow]")
                for svc in services:
                    enabled = svc.get("enabled", "false")
                    dot = "[green]●[/green]" if enabled == "true" else "[red]○[/red]"
                    svc_branch.add(
                        f"{dot} [cyan]{svc.get('name', '?')}[/cyan]"
                        f"  [dim]{svc.get('package', '?')}[/dim]"
                        f"  [dim]type={svc.get('type', '?')}[/dim]"
                        f"  [dim]recovery={svc.get('recovery', '?')}[/dim]"
                    )
            else:
                c_node.add("[dim red](no services)[/dim red]")

    console.print(root)


# ─── Tables ───────────────────────────────────────────────────────────────────

def list_containers_table(data: CommentedMap, output: str = "table") -> None:
    rows = []
    for spec in data.get("specs") or []:
        for c in spec.get("containers") or []:
            rows.append({
                "name": c.get("name", ""),
                "image": c.get("image", ""),
                "type": c.get("container-type", ""),
                "deploy-filepath": c.get("deploy-filepath", ""),
                "namespace": c.get("namespace", ""),
                "services": len(c.get("services") or []),
            })

    if output == "json":
        console.print_json(json.dumps(rows))
        return
    if output == "yaml":
        _print_yaml(rows)
        return

    table = Table(title="Containers", box=box.ROUNDED, show_lines=False)
    table.add_column("Name", style="bold green")
    table.add_column("Image")
    table.add_column("Type", style="cyan")
    table.add_column("Deploy Filepath")
    table.add_column("Namespace")
    table.add_column("# Services", justify="right")

    for r in rows:
        table.add_row(
            r["name"],
            r["image"],
            r["type"],
            r["deploy-filepath"] or "[dim]-[/dim]",
            r["namespace"] or "[dim]-[/dim]",
            str(r["services"]),
        )
    console.print(table)


def list_services_table(
    data: CommentedMap,
    container_filter: Optional[str] = None,
    output: str = "table",
) -> None:
    rows = []
    for spec in data.get("specs") or []:
        for c in spec.get("containers") or []:
            cname = c.get("name", "")
            if container_filter and cname != container_filter:
                continue
            for svc in c.get("services") or []:
                rows.append({
                    "container": cname,
                    "name": svc.get("name", ""),
                    "package": svc.get("package", ""),
                    "launchfile": svc.get("launchfile", ""),
                    "type": svc.get("type", ""),
                    "recovery": svc.get("recovery", ""),
                    "enabled": svc.get("enabled", "false"),
                    "max": svc.get("max", "0"),
                })

    if output == "json":
        console.print_json(json.dumps(rows))
        return
    if output == "yaml":
        _print_yaml(rows)
        return

    table = Table(title="Services", box=box.ROUNDED, show_lines=False)
    table.add_column("Container", style="bold green")
    table.add_column("Name", style="cyan")
    table.add_column("Package")
    table.add_column("Launchfile")
    table.add_column("Type")
    table.add_column("Recovery")
    table.add_column("Enabled", justify="center")
    table.add_column("Max", justify="right")

    for r in rows:
        enabled_str = (
            "[green]true[/green]" if r["enabled"] == "true" else "[red]false[/red]"
        )
        table.add_row(
            r["container"],
            r["name"],
            r["package"],
            r["launchfile"] or "[dim]-[/dim]",
            r["type"],
            r["recovery"],
            enabled_str,
            r["max"],
        )
    console.print(table)


# ─── Helpers ──────────────────────────────────────────────────────────────────

def _print_yaml(obj: object) -> None:
    buf = StringIO()
    yaml = YAML()
    yaml.default_flow_style = False
    yaml.dump(obj, buf)
    console.print(buf.getvalue())


def print_success(msg: str) -> None:
    console.print(f"[bold green]✔[/bold green] {msg}")


def print_info(msg: str) -> None:
    console.print(f"[bold blue]ℹ[/bold blue] {msg}")


def print_warning(msg: str) -> None:
    console.print(f"[bold yellow]⚠[/bold yellow] {msg}")


def print_error(msg: str) -> None:
    console.print(f"[bold red]✖[/bold red] {msg}")
