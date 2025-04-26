#include "SSD1306.h"
#include "SSD1306.hpp"
#include "string.h"
#include "i2c_bus.h"

#define SSD1306_COLOR_WHITE            0
#define SSD1306_COLOR_BLACK            1

#define I2C_SCL_PIN                 22
#define I2C_SDA_PIN                 21

#define SSD1306_BUFFER_SIZE            SSD1306_WIDTH * SSD1306_HEIGHT / 8


i2c_bus_device_t i2c_ssd1306 = {
    .dev_cfg = {
        .dev_addr_length    = I2C_ADDR_BIT_LEN_7,
        .device_address    = SSD1306_ADDR,
        .scl_speed_hz       = 100000,
    },
};

SSD1306 ssd1306(&i2c_ssd1306);
static uint8_t ssd1306Buffer[SSD1306_BUFFER_SIZE];

void SSD1306_main() {
    ssd1306.SSD1306_init();
    while(1) {
        ssd1306.SSD1306_clear(SSD1306_COLOR_BLACK);
        ssd1306.SSD1306_display();
        vTaskDelay(1000);
        ssd1306.SSD1306_clear(SSD1306_COLOR_WHITE);
        ssd1306.SSD1306_display();
        vTaskDelay(1000);
    }
}

ssd1306_err_t SSD1306::SSD1306_init() {
    volatile int ret = 0;
    esp_err_t status = i2c_bus_initDevice(&ssd1306.i2c_ssd1306->dev_cfg, &ssd1306.i2c_ssd1306->dev_handle);
    if (ESP_OK == status) {
        ssd1306.SSD1306_Cmd(SSD1306_DISPLAYOFF);

        ssd1306.SSD1306_Cmd(SSD1306_SETDISPLAYCLOCKDIV);
        ssd1306.SSD1306_Cmd(0x80);

        ssd1306.SSD1306_Cmd(SSD1306_HEIGHT - 1);

        ssd1306.SSD1306_Cmd(SSD1306_SETDISPLAYOFFSET);
        ssd1306.SSD1306_Cmd(0x00);
        ssd1306.SSD1306_Cmd(SSD1306_SETSTARTLINE);

        ssd1306.SSD1306_Cmd(SSD1306_CHARGEPUMP);
        ssd1306.SSD1306_Cmd(0x14);

        ssd1306.SSD1306_Cmd(SSD1306_MEMORYMODE); // 0x20
        ssd1306.SSD1306_Cmd(0x00); // 0x0 act like ks0108
        ssd1306.SSD1306_Cmd(SSD1306_SEGREMAP | 0x1);
        ssd1306.SSD1306_Cmd(SSD1306_COMSCANDEC);

        ssd1306.SSD1306_Cmd(SSD1306_SETCOMPINS);
        ssd1306.SSD1306_Cmd(0x12);
        ssd1306.SSD1306_Cmd(SSD1306_SETCONTRAST);
        ssd1306.SSD1306_Cmd(0xFF);

        ssd1306.SSD1306_Cmd(SSD1306_SETPRECHARGE); // 0xd9
        ssd1306.SSD1306_Cmd(0xF1);

        ssd1306.SSD1306_Cmd(SSD1306_SETVCOMDETECT); // 0xDB
        ssd1306.SSD1306_Cmd(0x40);
        ssd1306.SSD1306_Cmd(SSD1306_DISPLAYALLON_RESUME); // 0xA4
        ssd1306.SSD1306_Cmd(SSD1306_NORMALDISPLAY);       // 0xA6
        ssd1306.SSD1306_Cmd(SSD1306_DEACTIVATE_SCROLL);

        ssd1306.SSD1306_Cmd(SSD1306_DISPLAYON); // Main screen turn on
    }
    return ret;
}

void SSD1306::SSD1306_display() {
    ssd1306.SSD1306_Cmd(SSD1306_PAGEADDR);
    ssd1306.SSD1306_Cmd(0);                 // Page start address
    ssd1306.SSD1306_Cmd(7);              // 0xff Page end (not really, but works here)
    ssd1306.SSD1306_Cmd(SSD1306_COLUMNADDR);
    ssd1306.SSD1306_Cmd(0);                 // Column start address
    ssd1306.SSD1306_Cmd(SSD1306_WIDTH - 1);    // Column end address
    
    ssd1306.SSD1306_Data(ssd1306Buffer, SSD1306_BUFFER_SIZE);
}

void SSD1306::SSD1306_clear(uint8_t color) {
    switch(color){
        case SSD1306_COLOR_WHITE:
            memset(ssd1306Buffer, 0xFF, SSD1306_BUFFER_SIZE);
            break;
        case SSD1306_COLOR_BLACK:
            memset(ssd1306Buffer, 0x00, SSD1306_BUFFER_SIZE);
            break;
        default:
            break;
    }

}

ssd1306_err_t SSD1306::SSD1306_Cmd(uint8_t cmd) {
    uint8_t cmdBuf[2];
    cmdBuf[0] = SSD1306_CMD_ADDR;
    cmdBuf[1] = cmd;
    return i2c_bus_transmit(ssd1306.i2c_ssd1306->dev_handle, cmdBuf, 2, 100);
}

ssd1306_err_t SSD1306::SSD1306_Data(uint8_t *buffer, size_t size) {
    uint8_t dataBuf[SSD1306_BUFFER_SIZE + 1];
    dataBuf[0] = SSD1306_DATA_ADDR;
    memcpy(&dataBuf[1], buffer, SSD1306_BUFFER_SIZE);
    return i2c_bus_transmit(ssd1306.i2c_ssd1306->dev_handle, dataBuf, SSD1306_BUFFER_SIZE + 1, 200);
}