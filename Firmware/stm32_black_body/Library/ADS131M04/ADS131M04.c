#include "ADS131M04.h"
#include "stm32g4xx_hal_def.h"
#include <string.h>


void ADS131M04_Init(ADS131M04 *hdev){

    // Initalise pwm for adc clock input
    HAL_TIM_PWM_Start(hdev->clk_htim, hdev->clk_channel);
    __HAL_TIM_SET_COMPARE(hdev->clk_htim, hdev->clk_channel, clk_pwm_freq);

}


static HAL_StatusTypeDef SPI_TxRx(ADS131M04 *hdev, uint8_t *tx, uint8_t *rx, uint16_t len){

    HAL_GPIO_WritePin(hdev->cs_port, hdev->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hdev->hspi, tx, rx, len, 100);
    HAL_GPIO_WritePin(hdev->cs_port, hdev->cs_pin, GPIO_PIN_SET);
    return status;
}

static int32_t Sign_Extend_24(int32_t value){
    if (value & 0x00800000) value |= 0xFF000000;
    return value;
}

HAL_StatusTypeDef ADS131M04_Read_Reg(ADS131M04 *hdev, uint8_t address, uint16_t *data){

    // Initalise spi variables for write
    uint8_t rx[spi_bytes] = {0};
    uint8_t tx[spi_bytes] = {0};
    
    tx[0] = rreg | (address >> 1);
    tx[1] = (address << 7);
    
    //Send command to fetch data
    SPI_TxRx(hdev, tx, rx, spi_bytes);

    // Initalise spi variables for read
    uint8_t rx2[spi_bytes] = {0};
    uint8_t tx2[spi_bytes] = {0};

    // Read register data
    HAL_StatusTypeDef status = SPI_TxRx(hdev, tx2, rx2, spi_bytes);

    // Compile and return relavent data
    *data = ((uint16_t)rx2[0] << 8) | rx2[1];
    return status;
}


HAL_StatusTypeDef ADS131M04_Write_Reg(ADS131M04 *hdev, uint8_t address, uint16_t data){

    // Initalise spi variables for write
    uint8_t rx[spi_bytes] = {0};
    uint8_t tx[spi_bytes] = {0};
    
    // TX should have format {ADDRESS_0, ADDRESS_1, DATA_0, DATA_1}

    // Address
    tx[0] = wreg | (address >> 1);
    tx[1] = (address << 7);

    // Data
    tx[3] = (data >> 8); // MSB
    tx[4] = (data & 0xFF); // LSB

    //Send address and data
    return SPI_TxRx(hdev, tx, rx, spi_bytes);
}

HAL_StatusTypeDef ADS131M04_Reset(ADS131M04 *hdev){

    // Initalise spi variables for write
    uint8_t rx[spi_bytes] = {0};
    uint8_t tx[spi_bytes] = {0};
    
    // Address
    tx[1] = ADS_RESET;

    //Send address and data
    HAL_StatusTypeDef status = SPI_TxRx(hdev, tx, rx, spi_bytes);
    uint16_t data = ((uint16_t)rx[0] << 8) | rx[1];
    if (data == ADS_RESET_RESPONSE) {
        return status;
    } else {
        return HAL_ERROR;
    }
}


HAL_StatusTypeDef ADS131M04_ADC_Read(ADS131M04 *hdev, int32_t data[4]){
    uint8_t rx[spi_bytes] = {0};
    uint8_t tx[spi_bytes] = {0};

    HAL_StatusTypeDef status = SPI_TxRx(hdev, tx, rx, spi_bytes);

    if (status == HAL_OK){
        data[0] = Sign_Extend_24(((int32_t)rx[3] << 16) | (rx[4] << 8) | rx[5]);
        data[1] = Sign_Extend_24(((int32_t)rx[6] << 16) | (rx[7] << 8) | rx[8]);
        data[2] = Sign_Extend_24(((int32_t)rx[9] << 16) | (rx[10] << 8) | rx[11]);
        data[3] = Sign_Extend_24(((int32_t)rx[12] << 16) | (rx[13] << 8) | rx[14]);
    }

    return status;
}