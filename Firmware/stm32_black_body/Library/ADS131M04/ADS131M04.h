#pragma once

#include "stm32g431xx.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_spi.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_tim.h"

// PWM frequency for ADC clock input
// PWM must be set to prescaler 0 and counter period 20
#define clk_pwm_freq 10
#define spi_bytes 18

// read from register
#define rreg 0xA0

// write to register
#define wreg 0x60

// Reset command
#define ADS_RESET 0x11
// Reset response
#define ADS_RESET_RESPONSE 0xFF24

// Registers
#define ADS_ID 0x00
#define ADS_STATUS 0x01
#define ADS_MODE 0x02
#define ADS_CLOCK 0x03
#define ADS_GAIN 0x04
#define ADS_CFG 0x06
#define ADS_THRSHLD_MSB 0x07
#define ADS_THRSHLF_LSB 0x08
#define ADS_CH0_CFG 0x09
#define ADS_CH0_OCAL_MSB 0x0A
#define ADS_CH0_OCAL_LSB 0x0B
#define ADS_CH0_GCAL_MSB 0x0C
#define ADS_CH0_GCAL_LSB 0x0D
#define ADS_CH1_CFG 0x0E
#define ADS_CH1_OCAL_MSB 0x0F
#define ADS_CH1_OCAL_LSB 0x10
#define ADS_CH1_GCAL_MSB 0x11
#define ADS_CH1_GCAL_LSB 0x12
#define ADS_CH2_CFG 0x13
#define ADS_CH2_OCAL_MSB 0x14
#define ADS_CH2_OCAL_LSB 0x15
#define ADS_CH2_GCAL_MSB 0x16
#define ADS_CH2_GCAL_LSB 0x17
#define ADS_CH3_CFG 0x18
#define ADS_CH3_OCAL_MSB 0x19
#define ADS_CH3_OCAL_LSB 0x1A
#define ADS_CH3_GCAL_MSB 0x1B
#define ADS_CH3_GCAL_LSB 0x1C
#define ADS_REGMAP_CRC 0x3E
#define ADS_RESERVED 0x3F


/**
* @brief Struct to pass gpio interface data to ADS131M04 library
*/
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    TIM_HandleTypeDef *clk_htim;
    uint32_t clk_channel;
} ADS131M04;


/**
* @brief Initialises library pwm and spi
* @param hdev Struct which passes gpio data
* @return None
*/
void ADS131M04_Init(ADS131M04 *hdev);


/**
* @brief Reads data from register
* @param hdev Struct which passes gpio data
* @param address Register address to read data from (see ADS131M04.h)
* @param data Pointer to integer which updates with read register data
* @return SPI write status
*/
HAL_StatusTypeDef ADS131M04_Read_Reg(ADS131M04 *hdev, uint8_t address, uint16_t *data);

/**
* @brief Write data to register
* @param hdev Struct which passes gpio data
* @param address Register address to write data to (see ADS131M04.h)
* @param data Data to write to register
* @return SPI write status
*/
HAL_StatusTypeDef ADS131M04_Write_Reg(ADS131M04 *hdev, uint8_t address, uint16_t data);

/**
* @brief Reset device
* @param hdev Struct which passes gpio data
* @return SPI write and reset status
*/
HAL_StatusTypeDef ADS131M04_Reset(ADS131M04 *hdev);

/**
* @brief Reads ADC data
* @param hdev Struct which passes gpio data
* @param data Pointer to array which updates with ADC data
* @return SPI write status
*/
HAL_StatusTypeDef ADS131M04_ADC_Read(ADS131M04 *hdev, int32_t data[4]);