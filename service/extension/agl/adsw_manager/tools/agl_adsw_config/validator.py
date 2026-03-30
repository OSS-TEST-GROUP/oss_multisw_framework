from __future__ import annotations

from pathlib import Path
from typing import List, Tuple

from pydantic import ValidationError

from .model import ServiceList


def validate_file(path: Path) -> Tuple[bool, List[str]]:
    """Validate a YAML file against ServiceList schema.

    Returns (ok, list_of_error_messages).
    """
    try:
        from ruamel.yaml import YAML
        _yaml = YAML()
        with open(path, "r") as f:
            raw = _yaml.load(f)
        if raw is None:
            return False, ["File is empty or invalid YAML"]
        ServiceList.model_validate(raw)
        return True, []
    except ValidationError as e:
        errors = []
        for err in e.errors():
            loc = " -> ".join(str(x) for x in err["loc"])
            errors.append(f"{loc}: {err['msg']}")
        return False, errors
    except Exception as e:
        return False, [str(e)]
