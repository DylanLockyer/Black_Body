#pragma once

#include <Arduino.h>
#include "DAC60501.h"
#include <LittleFS.h>

// Wifi libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

// Wifi definitions
const char *WIFI_NAME = "Temperature Probe";

// I2C config
#define SDA_PIN 8
#define SCL_PIN 9

// Max ADC voltage output
#define ADC_MAX_VOLTAGE 2.5

// Max current output
#define MAX_CURRENT 0.5

struct SensorReading {
  volatile float temperature = NAN; // Kelvin, from GR-300-AA via profile interpolation
  volatile float resistance  = NAN; // Ohms, computed from voltage/current
  volatile float voltage     = 0.0f; // Volts
  volatile float current     = 0.0f; // Amps
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