# agl-adsw-config

CLI tool for managing `agl-service-list.yaml` — the service configuration file used by `agl/adsw_manager`.

## Install

```bash
cd tools/
pip install -e .
```

## Quick start

```bash
# Show all services as a tree
agl-adsw-config show

# List services as a table
agl-adsw-config list

# Get details for one service
agl-adsw-config get adsw_monitor

# Add a service
agl-adsw-config add my_svc --image my-app:latest --type NORMAL --recovery ALWAYS

# Update a field
agl-adsw-config set my_svc --recovery ON_FAILURE

# Enable / disable
agl-adsw-config enable  my_svc
agl-adsw-config disable my_svc

# Manage extra_args
agl-adsw-config args my_svc --add "--net=host"
agl-adsw-config args my_svc --list
agl-adsw-config args my_svc --remove "--net=host"

# Remove a service
agl-adsw-config remove my_svc

# Validate YAML structure
agl-adsw-config validate

# Create a timestamped backup
agl-adsw-config backup

# Diff against last backup
agl-adsw-config diff
```

## Global options

| Option | Default | Description |
|--------|---------|-------------|
| `-f, --file <path>` | `./config/agl-service-list.yaml` | Target YAML file |
| `--dry-run` | off | Preview changes without writing |
| `--no-backup` | off | Skip backup before saving |
| `-o, --output <fmt>` | `table` | `table \| json \| yaml` |

## YAML schema

```yaml
services:
  - name: adsw_monitor           # unique string
    service_type: NORMAL         # NORMAL | ONESHOT
    recovery_policy: ALWAYS      # R_DEFAULT | ALWAYS | ON_FAILURE | ON_SUCCESS
                                 # | ON_NOT_RESPONDING | MANUAL
    is_enabled: true             # true | false
    mw_type: AGL                 # AGL | ROS2 | AUTOSAR
    init_state: NOT_RUNNING      # NOT_RUNNING | RUNNING
    image: adsw-test-app:latest
    extra_args:
      - --net=host
```

## show output example

```
agl-adsw-config  v0.1
├── ● adsw_monitor  AGL  docker=adsw-test-app:latest  recovery=ALWAYS
├── ● map           AGL  docker=adsw-test-app:latest  recovery=R_DEFAULT
└── ○ radio         AGL  docker=adsw-test-app:latest  recovery=R_DEFAULT
```

(`●` = enabled, `○` = disabled)
