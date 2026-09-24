#pragma once

#include <stdbool.h>


// Resistance scaling
#define _10ua_shift 2.3
#define _10ua_scale 1
#define _100ua_shift 0.2
#define _100ua_scale 0.9029
#define _1ma_shift 0
#define _1ma_scale 0.66

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
bool current_source(current current_level, float *volt_scale, float *resistance_shift);
bool current_measurement_resistor(cur_resistor shunt_resistance);
bool current_direction(cur_direction direction);
