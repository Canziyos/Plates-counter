# Dish Counter Firmware

This directory contains the canonical ESP-IDF firmware. `Prototype_A` at the repository root is retained only as historical reference.

## Hardware assumptions

- ESP32 Heltec LoRa board with an SX127x radio
- Active-low IR tray sensor on GPIO 17
- Distance-sensor pulse input on GPIO 23
- SX127x SPI and interrupt pins as defined in `main/main.cpp`

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

## Current behavior

1. The IR task detects the leading edge of a tray and signals the measurement task.
2. The measurement task samples the distance pulse for up to ten seconds.
3. Five consecutive filtered samples inside the configured range produce one count.
4. Pulse waits time out, preventing a disconnected sensor from locking a FreeRTOS task.

LoRa transmission code exists but its task remains disabled in `main.cpp` until payload cadence and backend integration are validated.
