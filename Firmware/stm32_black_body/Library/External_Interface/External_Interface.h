#pragma once

// Libraries
#include "stm32g4xx_hal.h"
#include "stm32g431xx.h"
#include <string.h>
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_spi.h"
#include "switches.h"

#define INTERFACE_BYTES 16

typedef struct {
    float resistance;
    float voltage;
    float current;
    current cur_source;
    cur_resistor shunt_resistor;
    cur_direction cur_direction;
} Sensor_Data;

void external_interface_send(Sensor_Data *data, SPI_HandleTypeDef *hspi);