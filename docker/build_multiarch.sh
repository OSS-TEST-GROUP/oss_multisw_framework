#!/bin/bash

# Multi-architecture Docker image build script for OSS Multi-Middleware Framework

set -e

REGISTRY="registry.obigo.com"
BUILD_TYPE="${BUILD_TYPE:-Release}"
PLATFORMS="linux/amd64,linux/arm64"

echo "=== OSS Multi-Middleware Framework Multi-arch Build ==="
echo "Platforms: ${PLATFORMS}"
echo "Build Type: ${BUILD_TYPE}"
echo ""

# Create buildx builder if not exists
echo "Creating buildx builder..."
docker buildx create --name mmf-buildx --driver docker-container --use 2>/dev/null || \
docker buildx use mmf-buildx

echo ""
echo "=== Building MMF Core (multi-arch) ==="
docker buildx build \
    --platform="${PLATFORMS}" \
    --build-arg BUILD_TYPE=${BUILD_TYPE} \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-core \
    --push \
    -t ${REGISTRY}/oss/oss-mmf-core:latest \
    .

echo ""
echo "=== Building MMF Extension ROS2 (multi-arch) ==="
docker buildx build \
    --platform="${PLATFORMS}" \
    --build-arg BUILD_TYPE=${BUILD_TYPE} \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-ext-ros2 \
    --push \
    -t ${REGISTRY}/oss/oss-mmf-ext-ros2:latest \
    .

echo ""
echo "=== Building MMF Extension AGL (multi-arch) ==="
docker buildx build \
    --platform="${PLATFORMS}" \
    --build-arg BUILD_TYPE=${BUILD_TYPE} \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-ext-agl \
    --push \
    -t ${REGISTRY}/oss/oss-mmf-ext-agl:latest \
    .

echo ""
echo "=== Build Complete ==="
echo "Images pushed to ${REGISTRY}/oss/"
echo "  - oss-mmf-core:latest"
echo "  - oss-mmf-ext-ros2:latest"
echo "  - oss-mmf-ext-agl:latest"
echo ""
echo "Supported architectures: amd64, arm64"
