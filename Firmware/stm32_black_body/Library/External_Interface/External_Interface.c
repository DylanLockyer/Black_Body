#include "External_Interface.h"
#include "main.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_spi.h"

// XOR checksum over the payload bytes, used by the ESP32 to detect a
// transaction that was corrupted because the SPI slave wasn't armed in time
// (see STM32_Receive.cpp on the ESP32 side for the matching check).
static uint8_t checksum(const uint8_t *buf, size_t len){
    uint8_t chk = 0;
    for (size_t i = 0; i < len; i++){
        chk ^= buf[i];
    }
    return chk;
}

void external_interface_send(Sensor_Data *data, SPI_HandleTypeDef *hspi){
    uint8_t rx[INTERFACE_BYTES] = {0};
    uint8_t tx[INTERFACE_BYTES] = {0};
    memcpy(&tx[0], &data->resistance, sizeof(float));
    memcpy(&tx[4], &data->voltage, sizeof(float));
    memcpy(&tx[8], &data->current, sizeof(float));
    // source = s; direction = d, resistor = r; null = x;
    // tx[12] is sssddrrx
    tx[12] = (data->cur_source << 5) | ((data->cur_direction << 3) & 0x18) | ((data->shunt_resistor << 1) & 0x06);
    tx[13] = checksum(tx, INTERFACE_PAYLOAD_BYTES);

    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hspi, tx, rx, INTERFACE_BYTES, 100);
    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET);

    if (status != HAL_OK){
        HAL_SPI_Abort(hspi);
    }
}