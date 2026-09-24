# Device Usage Guide

How to power up, connect to, and operate the black body controller. For hardware/firmware design details, see the [root README](../README.md) and its links.

## Getting Started

![Delivered Product](Delivered%20Product.jpg)

The delivered unit is the 3D-printed enclosure (containing both PCBs), a 12V AC power adapter with mains cable, and a bag with the temperature probes and cables.

The enclosure has five connections on its edges:

| Label | Purpose |
|---|---|
| **Vin** | 12V DC input from the included power adapter |
| **SENSOR — I / V** | 4-wire connection to the germanium temperature probe: the **I** terminals carry the excitation current, the **V** terminals are the voltage sense leads |
| **HEATER** | The resistive heater element (max 12V/0.5A) |
| **USB COM** | USB port to the ESP32 heater/control board |
| **USB PROG** | USB port to the STM32 sensor board |

Before powering on, connect the temperature probe to **SENSOR** (I and V screw terminals) using the probe's datasheet to match pinout and the heater to **HEATER**, then plug the power adapter's output into **Vin**. The USB ports are only needed when reprogramming the firmware (see [Programming](#programming)) — they aren't used for normal operation. For the first power up after delivery it is recommended to use a resistor connected via 4 wire kelvin sensing to the Sensor I/V to ensure it wasn't damaged in transit.

## Powering On

Plug the adapter into mains power. There is no separate power switch — connecting mains power turns the unit on, and unplugging it turns it off. On power-up, the ESP32 begins broadcasting its WiFi access point and the STM32 sensor board starts pushing resistance/voltage/current readings to it over SPI; the readings become live in the web interface within a few seconds.

## Connecting to the Device

The controller hosts its own WiFi network — it does not join your existing WiFi. Connect to the access point:

- **Network name:** `Black Body`
- **Password:** none (open network)

Most phones and laptops will detect the captive portal and open the control page automatically. If it doesn't pop up, open a browser and go to:

```
http://192.168.4.1/
```

## Using the Web Interface

### Measure

![Measure tab](Screenshot%202026-09-23%20184508.png)

The left panel manages calibration **profiles** for the connected sensor: the default profile for the GR-300AA is automatically loaded. To pick a different saved profile, select it from the dropdown and use **LOAD** to make it active, **SAVE TO DEVICE** to store the currently-edited segments under a name, or **DELETE** to remove a saved profile. The segment table below it defines the resistance-to-temperature curve as a set of piecewise cubic segments (see [Calibration](#calibration) for how these are generated); use **+ SEGMENT** to add a row. The **DEBUG** panel (click **SHOW**) exposes the current excitation level, current direction, active shunt resistor, whether the calibration is valid, and which profile is currently active.

The right panel shows the live reading: temperature in large text, with resistance, voltage, and current underneath, plus a rolling 5-minute chart.

### Heat

![Heat tab](Screenshot%202026-09-23%20192509.png)

The left panel shows live temperature and a chart, plus a heater output readout (voltage drop, current, power, and percent of max power), and the **START/STOP** button that enables or disables closed-loop control.

The right panel sets the **target temperature** (sent with **SEND**) and the **PID gains** Kp/Ki/Kd (sent and saved to flash with **SEND & SAVE**). A **manual override** lets you drive the heater at a fixed current (0–0.5A) instead of running PID for testing which only works while the PID loop is stopped; the device will reject it otherwise.

## Calibration

Calibration profiles map measured sensor resistance to temperature as piecewise cubic segments of the form:

```
ln(T) = c0·ln(R)³ + c1·ln(R)² + c2·ln(R) + c3
```

with each segment applying over its own `[R min, R max)` range.

There are two ways to get the data for a new profile:

- **From the sensor's datasheet:** most calibrated sensors, including LakeShore probes, come with a resistance-vs-temperature table or published fit coefficients for that specific sensor. Use those values as the (resistance, temperature) pairs in step 3 below and skip the measurement steps. If the datasheet already gives piecewise fit coefficients in the same `ln(T)` vs `ln(R)` form, you can enter them directly (step 4).
- **By measuring the sensor yourself:** follow all the steps below.

To build a new profile for a sensor:

1. On the **Measure** tab, read the live resistance value as you take the probe through its usable temperature range (e.g. warming or cooling it slowly while checking its temperature against a reference).
2. Record pairs of (resistance, temperature) at enough points to span the full range you care about.
3. Give this list of resistance/temperature pairs to an AI assistant (e.g. Claude) and ask it to fit the data to the segmented curve form above, i.e. to return the `[R min, R max)` bounds and `c0`–`c3` coefficients for each segment needed to cover your data.
4. Enter the returned segments into the segment table on the Measure tab (use **+ SEGMENT** to add rows), then click **SAVE TO DEVICE** and give the profile a name.
5. Select the new profile from the dropdown and **LOAD** it to make it active.

## Programming

Reflashing either board requires opening the enclosure:

1. Unplug the power adapter, then remove the 5 M2 screws holding the enclosure lid and lift it off to access the boards' USB ports and BOOT/RESET buttons.
2. Reconnect the 12V power supply to **Vin**, and connect a USB cable from your PC to the port for the board you're programming — **USB COM** for the ESP32, **USB PROG** for the STM32.
3. On that board, hold down its **BOOT** button, briefly press and release **RESET**, then release **BOOT**. This forces the microcontroller into its USB bootloader instead of starting the existing firmware.
4. Flash the new firmware:
   - **ESP32** ([`Firmware/ESP32_heater`](../Firmware/ESP32_heater)): build and upload with PlatformIO (`pio run -t upload`, or the Upload button in the PlatformIO IDE). If you changed `data/website.html`, also run `pio run -t uploadfs` to update the web UI stored on the device.
   - **STM32** ([`Firmware/stm32_black_body`](../Firmware/stm32_black_body)): build using stm32 extension for VScode + cmake and flash with STM32CubeProgrammer over the USB PROG connection.
5. Unplug the unit, replace the lid, and reinstall the 5 M2 screws.

## Specifications

| | |
|---|---|
| **Input power** | 12V DC, via included adapter into **Vin** |
| **Heater element** | 4×10Ω in series (40Ω), analog current-source driven (not PWM) |
| **Heater output** | Up to ~12V / 0.3A peak, ~10W max |
| **Heater control** | Closed-loop PID, or manual current override (0–0.5A) while PID is stopped |
| **Sensor excitation current** | 10nA – 1mA DC in six decade steps (10nA, 100nA, 1µA, 10µA, 100µA, 1mA), switchable polarity |
| **Sensor ADC** | 24-bit (ADS131M04) |
| **Supported sensor** | LakeShore GR-300-AA germanium RTD, 4-wire |
| **Temperature range** | 2K – 100K. The GR-300-AA is rated down to 0.3K, but the low-temperature excitation limiting needed below ~2K was never calibrated, so that range is disabled |
| **Wireless interface** | Self-hosted WiFi AP `Black Body` (open), captive portal, `192.168.4.1` |
| **Board-to-board link** | SPI (ESP32 as slave, STM32 as master) |
| **Microcontrollers** | ESP32-S3 (heater/control board), STM32G431 (sensor board) |
| **Programming** | USB COM → ESP32 (PlatformIO), USB PROG → STM32 (STM32CubeProgrammer) |
