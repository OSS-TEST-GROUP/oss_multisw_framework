# adsw-config

CLI tool for managing `saved-service-list.yaml` in the `adsw_manager` project.

## Install

```bash
cd tools/
pip install -e .
```

## Usage

```
adsw-config [OPTIONS] COMMAND [ARGS]...
```

### Global Options

| Option | Default | Description |
|---|---|---|
| `-f / --file <path>` | `$SAVED_REGISTRY_BACKUP/saved-service-list.yaml` or `./config/saved-service-list.yaml` | Target YAML file |
| `--dry-run` | off | Preview changes without writing |
| `--no-backup` | off | Skip timestamped backup on write |
| `-o / --output` | `table` | Output format: `table` \| `json` \| `yaml` |

---

## Commands

### Inspect

```bash
# Full config tree
adsw-config show

# List all containers
adsw-config list containers

# List all services (optionally filter by container)
adsw-config list services
adsw-config list services --container my-container
```

### Container Management

```bash
# Add
adsw-config container add my-app --image my-app:latest --type docker
adsw-config container add my-compose --image my-app:latest --type docker-compose \
    --deploy-filepath docker-compose.yaml

# Update fields
adsw-config container set my-app --image my-app:2.0

# Remove (--force required if services exist)
adsw-config container remove my-app
adsw-config container remove my-app --force
```

### Service Management

```bash
# Add service
adsw-config service add my-container my_svc \
    --package my_pkg --launchfile my.launch.xml \
    --type NORMAL --recovery ALWAYS --enabled true

# Update fields
adsw-config service set my-container my_svc --recovery ON_FAILURE

# Enable / disable
adsw-config service enable my-container my_svc
adsw-config service disable my-container my_svc

# Remove
adsw-config service remove my-container my_svc
```

### File Management

```bash
# Validate structure and values
adsw-config validate

# Create a timestamped backup
adsw-config backup
# → saved-service-list.yaml.bak.20240101-120000

# Diff current file vs last backup
adsw-config diff
```

## YAML Schema

```yaml
version: 0.1
specs:
  - containers:
      - name: <string>          # required
        image: <string>         # required
        container-type: docker | docker-compose | k3s   # required
        deploy-filepath: <path> # required for docker-compose / k3s
        namespace: <ns>         # required for k3s
        services:
          - package: <pkg>      # required
            name: <string>      # required; unique within container
            launchfile: <file>
            type: NORMAL | ONESHOT
            recovery: ALWAYS | ON_FAILURE | ON_SUCCESS | ON_NOT_RESPONDING | MANUAL
            enabled: "true" | "false"
            max: "0"
```

## Environment Variables

| Variable | Description |
|---|---|
| `SAVED_REGISTRY_BACKUP` | Directory containing `saved-service-list.yaml` |
