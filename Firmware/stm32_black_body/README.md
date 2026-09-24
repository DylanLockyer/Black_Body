# STM32 Temperature Sensor Firmware

Firmware for the [temperature sensor PCB](../../Circuit%20Boards/Black_Body_Temperature_Sensor_PCB/README.md), targeting an STM32G431. Built with STM32CubeIDE / CMake (STM32CubeMX-generated HAL project). This board does the precision analog work — sourcing current into the sensor and reading it back — and pushes the result to the [ESP32](../ESP32_heater/README.md) for display and control.

## What It Does
- **Current source control:** `change_current()` selects the excitation current level and direction (10nA to 1mA, switchable polarity) by driving the analog switches on the sensor board.
- **External ADC readout:** Reads the 24-bit ADS131M04 ADC over SPI3 whenever its DRDY interrupt fires (`adc_flag`), capturing both the sensor voltage and the shunt voltage (used to derive current) for whichever shunt resistor (5Ω / 500Ω / 33kΩ) is currently selected.
- **Resistance calculation:** Computes sensor resistance from the averaged voltage and current readings (a rolling average smooths ADC noise before the divide).
- **Auto-ranging:** `autoset_current()` checks whether the current reading is in a good range for the selected sensor and switches current level/shunt automatically if not.
- **Push to ESP32:** Sends the latest voltage/current/resistance reading to the ESP32 over SPI2, with this board acting as SPI *master* (the ESP32 is the SPI slave and receives whatever this board clocks out).
- **USB:** USB 2.0 (`MX_USB_PCD_Init`) is used for programming and is reserved for a future PC-side application. Either USB or the ESP32 connector can supply board power, with protection against both being connected at once.
- **I2C:** Broken out for future expansion; unused by the current firmware logic.

## Layout
- `Core/Src/main.c` — peripheral init, the ADC read/current-switching main loop, and the SPI push to the ESP32.
- `Library/External_Interface` — SPI2 send routine used to talk to the ESP32.
- `Library/ADS131M04` — Communicates with SPI to the 24-bit adc. 
