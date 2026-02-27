# Auto Travel Router (ATR)

ATR is a self-hosted service for multi-day travel planning. It automatically clusters your Points of Interest (POIs) and generates near-optimal routes, helping you organize your trip with minimal effort.

The example provided in this repository is configured for **Berlin**.

## Demo

![demo video](docs/demo.gif)

## Self-Hosting & Portability

ATR is a standalone stack. While configured for Berlin by default, it can be deployed for any region:

1. Download an OpenStreetMap PBF file (e.g., from [Geofabrik](https://download.geofabrik.de/)).
2. Rename the file to `city.osm.pbf`.
3. Rebuild the stack to generate a new routing graph from your dataset.

## Setup

### Docker

1. Place `city.osm.pbf` in the root folder.
2. Build and start the containers:

    ```bash
    docker compose up --build -d
    ```

3. Access the service at `http://localhost`.

### Local Development (Non-Docker)

If you prefer to build the project directly on your system, you will need:

* **Compiler:** A C++20 compatible compiler
* **Tools:** CMake 3.20+ and GBuild/Ninja
* **Libraries:**
  * [Boost](https://www.boost.org/) (`context` and `coroutine`)
  * [libosmium](https://github.com/osmcode/libosmium)
  * [protozero](https://github.com/mapbox/protozero)
  * [zlib](https://www.zlib.net/)

## Interaction

* **Destinations:** Click the map to add POIs.
* **Hotel:** `Shift + Click` to set the starting and ending point for your days.
* **Visualization:** Grey dots represent click locations; colored dots represent the actual snapped network nodes.
