#include "External_Interface.h"
#include "main.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_spi.h"

void external_interface_send(Sensor_Data *data, SPI_HandleTypeDef *hspi){
    uint8_t rx[INTERFACE_BYTES] = {0};
    uint8_t tx[INTERFACE_BYTES] = {0};
    memcpy(&tx[0], &data->resistance, sizeof(float));
    memcpy(&tx[4], &data->voltage, sizeof(float));
    memcpy(&tx[8], &data->current, sizeof(float));
    // source = s; direction = d, resistor = r; null = x;
    // tx[12] is sssddrrx
    tx[12] = (data->cur_source << 5) | ((data->cur_direction << 3) & 0x18) | ((data->shunt_resistor << 1) & 0x06);

    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hspi, tx, rx, INTERFACE_BYTES, 20);
    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET);

    if (status != HAL_OK){
        HAL_SPI_Abort(hspi);
    }
}