#pragma once
#include <stdbool.h>
#include "main.h"

// Type for switching current source
typedef enum {
    cur_10na,
    cur_100na,
    cur_1ua,
    cur_10ua,
    cur_100ua,
    cur_1ma,
    cur_block
} current;


typedef enum {
    none,
    _5,
    _500,
    _33k2,
    disable
} cur_resistor;


typedef enum {
    left,
    right,
    off
} cur_direction;


// Expose current source adjustment settings
bool current_source(current current_level);
bool current_measurment_resistor(cur_resistor shunt_resistance);
bool current_direction(cur_direction direction);
