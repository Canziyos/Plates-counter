# Dish Counter Firmware

This directory contains the repository's only buildable ESP-IDF firmware. The retired three-IR-sensor design is preserved as a [historical note](../docs/historical-prototype-a.md).

## Hardware assumptions

- ESP32 Heltec LoRa board with an SX127x radio
- Active-low IR tray sensor on GPIO 17
- Distance-sensor pulse input on GPIO 23
- SX127x SPI and interrupt pins as defined in `main/app_config.h`

Verify voltage levels and pin assignments against the exact board and sensor revisions before powering the system.

## Build

Use an ESP-IDF environment compatible with the vendored `ttn-esp32` component:

```sh
cd firmware
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

Under **Dish counter configuration**, provide the DevEUI, AppEUI and AppKey. Credentials are intentionally excluded from source control.

Successful GitHub Actions builds publish an unprovisioned firmware bundle containing `dish_counter.bin`, the ESP32 bootloader, the partition table, flash-argument metadata and SHA-256 checksums. Download it from the run's **Artifacts** section. For a TTN-connected device, build locally after supplying credentials through `menuconfig`.

## Current behavior

1. The IR task detects the leading edge of a tray and signals the measurement task.
2. The measurement task samples the distance pulse for up to ten seconds.
3. Five consecutive filtered samples inside the configured range produce one count.
4. Pulse waits time out, preventing a disconnected sensor from locking a FreeRTOS task.

LoRaWAN provisioning and join retries run independently of dish counting. Telemetry transmission remains intentionally unimplemented until payload cadence, format and backend integration are validated.

See the repository's [architecture document](../docs/architecture.md) for module ownership, failure isolation, configuration locations and verification limits.
