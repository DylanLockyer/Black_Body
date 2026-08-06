#ifndef STM32_Receive_H
#define STM32_Receive_H

#include <Arduino.h>
#include "driver/spi_slave.h"
#include "esp_heap_caps.h"

// Temp Probe spi cs pin
#define SPI_CS 39
#define SPI_SCK 41
#define SPI_MISO 42
#define SPI_MOSI 40

#define SPI_BYTES 16

#define SPI_HOST SPI2_HOST
/* Must match the STM32-side layout exactly (packed, same field order). */
// Type for switching current source
enum current_source {
    cur_10na,
    cur_100na,
    cur_1ua,
    cur_10ua,
    cur_100ua,
    cur_1ma,
    cur_block
};


enum cur_resistor {
    none,
    _5,
    _500,
    _33k2,
    disable
};


enum cur_direction {
    left,
    right,
    off
};

struct Sensor_Data {
    float resistance;
    float voltage;
    float current;
    current_source cur_source;
    cur_resistor shunt_resistor;
    cur_direction current_direction;
};

class STM32Sensor {
public:
    STM32Sensor(uint8_t csPin);

    void begin();
    bool read(Sensor_Data *data);

private:
    uint8_t _cs;

    uint8_t *_rxBuffer;
    uint8_t *_txBuffer;
};


#endif // STM32_SENSOR_H