#ifndef _I2C_BUS_H
#define _I2C_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "oled_controller.h"

#include "driver/i2c_master.h"

typedef struct {
    uint8_t                     dev_name[32];
    i2c_device_config_t         dev_cfg;
    i2c_master_dev_handle_t     dev_handle;

    uint8_t                     *tx_buffer;
    uint8_t                     *rx_buffer;
    size_t                      tx_bufferSize;
    size_t                      rx_bufferSize;
} i2c_bus_device_t;

esp_err_t i2c_bus_initDevice(i2c_device_config_t *dev_cfg, i2c_master_dev_handle_t *dev_handle);
esp_err_t i2c_bus_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int timeout);
esp_err_t i2c_bus_receive(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size, int timeout);

#ifdef __cplusplus
}
#endif


#endif