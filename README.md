# Automated Cafeteria Dish Counter

An ESP32 proof of concept for counting trays or dishes passing through a cafeteria washing station. An IR sensor starts a measurement window, a pulse-output distance sensor provides classification data, and FreeRTOS tasks coordinate the workflow. The firmware also contains an experimental LoRaWAN integration for The Things Network.

## Repository layout

```text
.
├── firmware/           # Canonical ESP-IDF firmware
├── tests/              # Hardware-independent logic tests
├── tools/              # Desktop trace simulator
├── examples/traces/    # Synthetic sensor traces
└── docs/               # Architecture and historical design notes
```

Start with [`firmware/README.md`](firmware/README.md) for hardware assumptions, configuration and build commands.
Use the [`tools/` simulator](tools/README.md) to exercise counting decisions without an ESP32.
The retired three-IR-sensor design is documented in [`docs/historical-prototype-a.md`](docs/historical-prototype-a.md).

## Status

This is an unfinished hardware prototype, not a production counting system. The core tray-triggered measurement path has timeout protection and consecutive-sample counting, but it still needs hardware validation, calibrated thresholds, persistent count storage and end-to-end LoRaWAN testing.

LoRaWAN credentials must be supplied through `idf.py menuconfig`; they are not stored in the repository.
