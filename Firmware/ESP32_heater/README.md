# ESP32 Heater Firmware

Firmware for the [heater control PCB](../../Circuit%20Boards/Black_Body_Heater_Control_PCB/README.md). Built with PlatformIO (Arduino framework for ESP32). This is the "master" of the system: it drives the heater, talks to the STM32 sensor board over SPI, and hosts the web interface used to operate the device — see the [device usage guide](../../Documentation/README.md).

## What It Does
- **WiFi access point + web UI:** Starts a self-hosted WiFi AP (`WiFi.softAP`) with a DNS captive portal and an async web server that serves the control page (`data/website.html`) and a JSON API for live readings and commands.

  ![Example UI](../../Documentation/Example_Website.png)

- **Sensor readout over SPI:** Acts as an SPI slave, receiving voltage/current/resistance readings (`STM32Sensor`) pushed by the STM32 sensor board, which drives the SPI clock as master.
- **Resistance → temperature calibration:** Maintains per-sensor calibration "profiles" (piecewise cubic segments mapping resistance to temperature, see `SegmentedCalibration.h`), editable from the web UI and persisted to flash (LittleFS) so no on-device curve fitting is needed.
- **Heater PID control:** Runs a PID loop targeting a user-set temperature, or a manual power override, and drives the heater's analog current source through a DAC (`DAC60501`). PID gains and the active setpoint are persisted to flash.

## Task Structure
The firmware splits work across the ESP32's two cores to keep the SPI link and WiFi stack from blocking each other:
- **Core 0 — `sensorTask`:** Polls the STM32 over SPI as fast as possible and updates the shared reading state under `dataMutex`.
- **Core 1 — `loop()`:** Arduino's main loop, handling only WiFi/DNS captive-portal housekeeping.
- **Core 1 — `heaterTask`:** Runs the PID/manual-override heater control loop at ~5Hz, independent of the networking loop.

## Layout
- `src/main.cpp` — setup, FreeRTOS tasks, PID control, calibration/config persistence.
- `include/main.h` — pin definitions and shared types.
- `data/website.html` — the web UI served to clients connected to the AP.
