"""Pydantic v2 models for adsw-config schema validation."""
from __future__ import annotations

from enum import Enum
from typing import List, Optional

from pydantic import BaseModel, ConfigDict, Field, model_validator


class ContainerType(str, Enum):
    DOCKER = "docker"
    DOCKER_COMPOSE = "docker-compose"
    K3S = "k3s"


class ServiceType(str, Enum):
    NORMAL = "NORMAL"
    ONESHOT = "ONESHOT"


class RecoveryType(str, Enum):
    ALWAYS = "ALWAYS"
    ON_FAILURE = "ON_FAILURE"
    ON_SUCCESS = "ON_SUCCESS"
    ON_NOT_RESPONDING = "ON_NOT_RESPONDING"
    MANUAL = "MANUAL"


class ServiceSpec(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    package: str
    name: str
    launchfile: str = ""
    type: ServiceType = ServiceType.NORMAL
    recovery: RecoveryType = RecoveryType.ALWAYS
    enabled: str = "true"
    max: str = "0"


class ContainerSpec(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    name: str
    image: str
    container_type: ContainerType = Field(..., alias="container-type")
    deploy_filepath: Optional[str] = Field(None, alias="deploy-filepath")
    namespace: Optional[str] = None
    services: List[ServiceSpec] = []

    @model_validator(mode="after")
    def check_requirements(self) -> ContainerSpec:
        if self.container_type in (ContainerType.DOCKER_COMPOSE, ContainerType.K3S):
            if not self.deploy_filepath:
                raise ValueError(
                    f"deploy-filepath is required for container-type '{self.container_type.value}'"
                )
        if self.container_type == ContainerType.K3S:
            if not self.namespace:
                raise ValueError("namespace is required for container-type 'k3s'")
        return self


class Spec(BaseModel):
    containers: List[ContainerSpec] = []


class Config(BaseModel):
    version: float = 0.1
    specs: List[Spec] = []
