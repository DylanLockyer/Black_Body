#include "STM32_Receive.h"

STM32Sensor::STM32Sensor(uint8_t csPin)
    : _cs(csPin),
      _rxBuffer(nullptr),
      _txBuffer(nullptr)
{
}

void STM32Sensor::begin()
{
    // ESP32 is the SPI SLAVE.
    // STM32 controls CS, so CS must be an input.
    pinMode(_cs, INPUT);

    // SPI bus configuration
    spi_bus_config_t buscfg = {};

    buscfg.mosi_io_num = SPI_MOSI;
    buscfg.miso_io_num = SPI_MISO;
    buscfg.sclk_io_num = SPI_SCK;

    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    buscfg.max_transfer_sz = SPI_BYTES;

    // SPI slave configuration
    spi_slave_interface_config_t slvcfg = {};

    slvcfg.spics_io_num = _cs;
    slvcfg.flags = 0;
    slvcfg.queue_size = 1;

    // SPI_MODE0:
    // CPOL = 0
    // CPHA = 0
    slvcfg.mode = 1;

    // Initialize SPI slave
    esp_err_t ret = spi_slave_initialize(
        SPI_HOST,
        &buscfg,
        &slvcfg,
        SPI_DMA_CH_AUTO
    );

    if (ret != ESP_OK) {
        Serial.printf(
            "SPI slave initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        while (true) {
            delay(1000);
        }
    }

    // DMA-capable buffers
    _rxBuffer = (uint8_t *)heap_caps_malloc(
        SPI_BYTES,
        MALLOC_CAP_DMA
    );

    _txBuffer = (uint8_t *)heap_caps_malloc(
        SPI_BYTES,
        MALLOC_CAP_DMA
    );

    if (_rxBuffer == nullptr || _txBuffer == nullptr) {
        Serial.println("Failed to allocate SPI DMA buffers");

        while (true) {
            delay(1000);
        }
    }

    memset(_rxBuffer, 0, SPI_BYTES);
    memset(_txBuffer, 0, SPI_BYTES);

    Serial.println("SPI slave initialized");
}

bool STM32Sensor::read(Sensor_Data *data)
{
    if (_rxBuffer == nullptr || _txBuffer == nullptr) {
        return false;
    }
    //if (digitalRead(_cs) == HIGH) return false;

    // Clear buffers before every transaction
    memset(_rxBuffer, 0, SPI_BYTES);
    memset(_txBuffer, 0, SPI_BYTES);

    spi_slave_transaction_t trans = {};

    // 13 bytes = 104 bits
    trans.length = SPI_BYTES * 8;

    trans.rx_buffer = _rxBuffer;
    trans.tx_buffer = _txBuffer;

    // SEnd data
    esp_err_t ret = spi_slave_transmit(
        SPI_HOST,
        &trans,
        5
    );

    if (ret != ESP_OK) {
        Serial.printf(
            "SPI receive failed: %s\n",
            esp_err_to_name(ret)
        );

        return false;
    }

    memcpy(&data->resistance, &_rxBuffer[0], sizeof(float));
    memcpy(&data->voltage, &_rxBuffer[4], sizeof(float));
    memcpy(&data->current, &_rxBuffer[8], sizeof(float));
    data->cur_source = (current_source)((_rxBuffer[12] >> 5) & 0x07);
    data->current_direction = (cur_direction)((_rxBuffer[12] >> 3) & 0x3);
    data->shunt_resistor = (cur_resistor)((_rxBuffer[12]>>1) & 0x03);
    Serial.println(data->resistance);
    return true;
}