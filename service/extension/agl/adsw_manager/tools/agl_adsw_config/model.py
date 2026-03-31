from __future__ import annotations

from typing import List, Optional
from enum import Enum

from pydantic import BaseModel, Field, model_validator


class ServiceType(str, Enum):
    NORMAL = "NORMAL"
    ONESHOT = "ONESHOT"


class RecoveryPolicy(str, Enum):
    R_DEFAULT = "R_DEFAULT"
    ALWAYS = "ALWAYS"
    ON_FAILURE = "ON_FAILURE"
    ON_SUCCESS = "ON_SUCCESS"
    ON_NOT_RESPONDING = "ON_NOT_RESPONDING"
    MANUAL = "MANUAL"


class OsType(str, Enum):
    AGL = "AGL"
    ROS2 = "ROS2"
    AUTOSAR = "AUTOSAR"


class InitState(str, Enum):
    NOT_RUNNING = "NOT_RUNNING"
    RUNNING = "RUNNING"


class ServiceEntry(BaseModel):
    name: str
    service_type: ServiceType = ServiceType.NORMAL
    recovery_policy: RecoveryPolicy = RecoveryPolicy.R_DEFAULT
    is_enabled: bool = True
    os_type: OsType = OsType.AGL
    init_state: InitState = InitState.NOT_RUNNING
    image: str
    extra_args: List[str] = Field(default_factory=list)


class ServiceList(BaseModel):
    services: List[ServiceEntry] = Field(default_factory=list)

    @model_validator(mode="after")
    def check_unique_names(self) -> "ServiceList":
        names = [s.name for s in self.services]
        duplicates = {n for n in names if names.count(n) > 1}
        if duplicates:
            raise ValueError(f"Duplicate service name(s): {', '.join(sorted(duplicates))}")
        return self
