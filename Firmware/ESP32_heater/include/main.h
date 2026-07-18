#pragma once

#include <Arduino.h>
#include "DAC60501.h"
#include <LittleFS.h>
#include "FS.h"
// I2C config
#define SDA_PIN 8
#define SCL_PIN 9

// Max ADC voltage output
#define ADC_MAX_VOLTAGE 2.5


// Shunt amplifier goes to GPIO13
const int current_pin = 13; 

// Voltage measuring resistor divider goes to GPIO12
const int voltage_pin = 12;

void littlefs_setup();
