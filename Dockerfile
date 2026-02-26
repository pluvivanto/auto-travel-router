FROM debian:stable-slim AS builder
WORKDIR /app

ARG DEBIAN_FRONTEND=noninteractive
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update \
    && apt-get install -y \
    build-essential cmake git perl libboost-context-dev libboost-coroutine-dev zlib1g-dev libosmium2-dev

COPY . .

ARG BUILD_TYPE=Release
RUN --mount=type=cache,target=/app/build \
    make BUILD_TYPE=${BUILD_TYPE} BUILD_TESTING=OFF build \
    && mkdir -p /app/dist \
    && cp build/src/atr_server /app/dist/atr_server

FROM debian:stable-slim
WORKDIR /app

RUN apt-get update && apt-get install -y \
    libboost-context-dev libboost-coroutine-dev zlib1g-dev libosmium2-dev \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd --system runner && useradd --system --gid=runner runner
USER runner

COPY --from=builder /app/dist/atr_server .
COPY city.osm.pbf .

EXPOSE 8080
ENTRYPOINT ["/app/atr_server", "city.osm.pbf"]
