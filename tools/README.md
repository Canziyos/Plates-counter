# Desktop dish-counter simulator

The simulator feeds recorded or synthetic raw distance samples through the same Kalman filter and consecutive-sample classifier used by the ESP32 firmware. It does not emulate GPIO timing, FreeRTOS scheduling, the physical sensors or LoRaWAN.

## Build and run

With CMake:

```sh
cmake -S tests -B tests/build
cmake --build tests/build
./tests/build/dish_counter_simulator examples/traces/clean-count.trace
```

On Windows with a multi-configuration CMake generator, the executable may instead be at `tests/build/Debug/dish_counter_simulator.exe` or `tests/build/Release/dish_counter_simulator.exe`.

The trace format is intentionally small:

```text
tray       # start a measurement window
225        # raw distance sample
-1         # sensor timeout or invalid pulse
end        # finish and report the tray
```

Empty lines and lines beginning with `#` are ignored. A file may contain multiple tray windows. If the final `end` is omitted, end-of-file closes the active tray automatically.

Start with the traces under [`examples/traces/`](../examples/traces/), then replace their values with measurements captured from real hardware when it becomes available.
