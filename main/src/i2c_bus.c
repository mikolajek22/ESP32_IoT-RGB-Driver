
#include "i2c_bus.h"
#define I2C_SCL_PIN                 22
#define I2C_SDA_PIN                 21

i2c_master_bus_handle_t i2c_mst_busHandler;

esp_err_t i2c_bus_init() {

    i2c_master_bus_config_t i2c_mst_config = {0};

    i2c_mst_config.clk_source           = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port             = I2C_NUM_0;
    i2c_mst_config.scl_io_num           = I2C_SCL_PIN;
    i2c_mst_config.sda_io_num           = I2C_SDA_PIN;
    i2c_mst_config.glitch_ignore_cnt    = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    return i2c_new_master_bus(&i2c_mst_config, &i2c_mst_busHandler);
}

esp_err_t i2c_bus_initDevice(i2c_device_config_t *dev_cfg, i2c_master_dev_handle_t *dev_handle) {
    i2c_bus_init();
    return i2c_master_bus_add_device(i2c_mst_busHandler, dev_cfg, dev_handle);
}

esp_err_t i2c_bus_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int timeout) {
    return i2c_master_transmit(i2c_dev, write_buffer, write_size, timeout);
}

esp_err_t i2c_bus_receive(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size, int timeout) {
    return i2c_master_receive(i2c_dev, read_buffer, read_size, timeout);
}