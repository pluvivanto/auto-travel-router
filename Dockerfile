FROM debian:stable-slim AS builder
WORKDIR /app

ARG DEBIAN_FRONTEND=noninteractive
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update \
    && apt-get install -y \
    build-essential cmake git perl libboost-all-dev zlib1g-dev libosmium2-dev libprotozero-dev

COPY . .

RUN --mount=type=cache,target=/app/build \
    cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    && cmake --build build --parallel $(nproc) \
    && mkdir -p /app/dist \
    && cp build/src/atr_server /app/dist/atr_server

FROM debian:stable-slim
WORKDIR /app

RUN apt-get update && apt-get install -y \
    libboost-all-dev zlib1g-dev libosmium2-dev libprotozero-dev \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd --system runner && useradd --system --gid=runner runner
USER runner

COPY --from=builder /app/dist/atr_server .
COPY city.osm.pbf .

EXPOSE 8080
ENTRYPOINT ["/app/atr_server", "city.osm.pbf"]
