# brake-ecu example image (reference).
#
# Illustrative Dockerfile showing how a developer ECU is packaged: it is the same
# shape as the runtime-mmf-sils-vecu stages in docker/Dockerfile.rel, but ships the
# developer's own binary + entrypoint. Two builds in one file:
#
#   builder  -- configure the repo with BUILD_SILS so idl/dds is generated and the
#               sils_vecu SDK + brake_ecu compile (brake_ecu is added to the SiLS build
#               via SILS_BUILD_EXAMPLES=ON). An out-of-tree ECU would instead FROM a
#               published mmf-sils-builder image and find_package(sils_vecu).
#   runtime  -- copy just the brake_ecu binary + entrypoint onto a slim base.
#
# Build from the repo ROOT (context must include idl/, service/, docker/):
#   docker build -f service/sils/examples/brake-ecu/Dockerfile -t brake-ecu:dev .

# ---- builder ---------------------------------------------------------------
FROM ubuntu:24.04 AS builder
# (Illustrative: install the same toolchain + FastDDS the core builder uses. In this
# repo you would reuse the project's builder base image instead of re-listing deps.)
ARG BUILD_TYPE=Release
WORKDIR /src
COPY . .
RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DBUILD_SILS=TRUE \
        -DSILS_BUILD_EXAMPLES=ON \
    && cmake --build build --target brake_ecu -j \
    && cmake --install build

# ---- runtime ---------------------------------------------------------------
FROM ubuntu:24.04 AS runtime
COPY --from=builder /usr/local/bin/brake_ecu /oss/bin/brake_ecu
COPY --from=builder /usr/local/lib /oss/lib
COPY service/sils/examples/brake-ecu/entrypoint.sh /oss/entrypoint.sh
RUN chmod +x /oss/entrypoint.sh
# Single-process container: the ECU IS the container (no supervised fan-out).
ENTRYPOINT ["/oss/entrypoint.sh"]
