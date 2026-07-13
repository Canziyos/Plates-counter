# Automated Cafeteria Dish Counter

An ESP32 proof of concept for counting trays or dishes passing through a cafeteria washing station. An IR sensor starts a measurement window, a pulse-output distance sensor provides classification data, and FreeRTOS tasks coordinate the workflow. The firmware also contains an experimental LoRaWAN integration for The Things Network.

## Status

This is an unfinished hardware prototype. The core tray-triggered measurement path has timeout protection and consecutive-sample counting, but it still needs hardware validation, calibrated thresholds, persistent count storage and end-to-end LoRaWAN testing.

LoRaWAN credentials must be supplied through `idf.py menuconfig`.
