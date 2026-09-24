#include "STM32_Receive.h"

// XOR checksum over the payload bytes, matching External_Interface.c on the
// STM32 side. Lets us detect (and drop) a transaction that got corrupted
// because the SPI slave peripheral wasn't armed yet when the STM32 master
// started clocking -- which otherwise shows up as an occasional wild,
// out-of-nowhere reading.
static uint8_t checksum(const uint8_t *buf, size_t len)
{
    uint8_t chk = 0;
    for (size_t i = 0; i < len; i++) {
        chk ^= buf[i];
    }
    return chk;
}

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
    memset(_rxBuffer, 1, SPI_BYTES);
    memset(_txBuffer, 0, SPI_BYTES);

    spi_slave_transaction_t trans = {};

    // 13 bytes = 104 bits
    trans.length = SPI_BYTES * 8;

    trans.rx_buffer = _rxBuffer;
    trans.tx_buffer = _txBuffer;

    // SEnd data
    // Diagnostic timeout: long enough that a slow/jittery STM32 master
    // won't cause spurious failures, short enough to get a "still no
    // transaction" log roughly every 100ms if the master isn't talking
    // at all. Once confirmed working, switch to portMAX_DELAY (this task
    // has nothing else to do while waiting for the master).
    esp_err_t ret = spi_slave_transmit(
        SPI_HOST,
        &trans,
        pdMS_TO_TICKS(100)
    );

    if (ret != ESP_OK) {
        //Serial.printf(
            //"SPI receive failed: %s\n",
            //esp_err_to_name(ret)
        //);

        return false;
    }

    // Reject a transaction whose checksum doesn't match: the STM32 master
    // can start clocking a transfer in the small window before this task
    // re-arms spi_slave_transmit, which shifts out stale/undefined bytes
    // instead of the real frame. Silently dropping it (rather than
    // publishing it) is what stops those as random spikes in the reading.
    if (checksum(_rxBuffer, SPI_PAYLOAD_BYTES) != _rxBuffer[SPI_PAYLOAD_BYTES]) {
        return false;
    }

    memcpy(&data->resistance, &_rxBuffer[0], sizeof(float));
    memcpy(&data->voltage, &_rxBuffer[4], sizeof(float));
    memcpy(&data->current, &_rxBuffer[8], sizeof(float));
    data->cur_source = (current_source)((_rxBuffer[12] >> 5) & 0x07);
    data->current_direction = (cur_direction)((_rxBuffer[12] >> 3) & 0x3);
    data->shunt_resistor = (cur_resistor)((_rxBuffer[12]>>1) & 0x03);
    return true;
}