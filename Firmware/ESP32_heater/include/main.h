#pragma once

#include <Arduino.h>
#include "DAC60501.h"
#include <LittleFS.h>
#include "STM32_Receive.h"
#include "CurveFit.h"

// Wifi libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

// Wifi definitions
const char *WIFI_NAME = "Black Body";

// I2C config
#define SDA_PIN 8
#define SCL_PIN 9

// Max ADC voltage output
#define ADC_MAX_VOLTAGE 2.5

// Max current output
#define MAX_CURRENT 0.3

// Upper end of the get_voltage() ADC mapping range; also used as the
// reference for MAX_HEATER_POWER_W below.
#define HEATER_VOLTAGE_FULL_SCALE 36.3f

// Used as the denominator for the "% of max power" readout on the heat
// page. This is an estimate (full-scale current x full-scale voltage),
// not a measured heater rating, since the true max power depends on the
// heater's resistance.
#define MAX_HEATER_POWER_W (MAX_CURRENT * 12.0f)


struct SensorReading {
  volatile float temperature = NAN; // Kelvin, from GR-300-AA via profile interpolation
  volatile float resistance  = NAN; // Ohms, computed from voltage/current
  volatile float voltage     = 0.0f; // Volts
  volatile float current     = 0.0f; // Amps

  uint8_t curSource;
  uint8_t curDirection;
  uint8_t shuntResistor;
};

// Heater board feedback + last commanded output, published by heaterTask.
struct HeaterReading {
  volatile float voltage       = 0.0f; // Volts, from get_voltage()
  volatile float current       = 0.0f; // Amps, from get_current()
  volatile float outputCurrent = 0.0f; // Amps, last value passed to set_output_current()
};

// Heater control setpoints, written from the web server's request handlers
// and read by heaterTask. Guarded by dataMutex, same as latestReading.
struct HeaterControl {
  volatile bool  running            = false; // PID loop active
  volatile float targetTemperature  = NAN;   // Kelvin; NOT persisted to flash
  volatile bool  manualOverride     = false; // only honored while !running
  volatile float manualCurrent      = 0.0f;  // Amps, 0..MAX_CURRENT

  // Persisted to LittleFS (see PID_CONFIG_FILE in main.cpp) and restored on boot.
  float pidKp = 2.0f;
  float pidKi = 0.1f;
  float pidKd = 0.0f;
};

// Shunt amplifier goes to GPIO13
const int current_pin = 13; 

// Voltage measuring resistor divider goes to GPIO12
const int voltage_pin = 12;

void wifi_setup();

String get_html();

float get_current();

float get_voltage();

float map(float x, float x_min, float x_max, float y_min, float y_max);

void set_output_current(float current);