#include "SSD1306.h"
#include "SSD1306.hpp"
#include "string.h"
#include "i2c_bus.h"

#define I2C_SCL_PIN                 22
#define I2C_SDA_PIN                 21

uint8_t SSD1306::ssd1306Buffer[SSD1306_BUFFER_SIZE];


ssd1306_err_t SSD1306::SSD1306_init() {
    volatile int ret = 0;
    esp_err_t status = i2c_bus_initDevice(&this->i2c_ssd1306->dev_cfg, &this->i2c_ssd1306->dev_handle);
    if (ESP_OK == status) {
        this->SSD1306_Cmd(SSD1306_DISPLAYOFF);

        this->SSD1306_Cmd(SSD1306_SETDISPLAYCLOCKDIV);
        this->SSD1306_Cmd(0x80);

        this->SSD1306_Cmd(SSD1306_HEIGHT - 1);

        this->SSD1306_Cmd(SSD1306_SETDISPLAYOFFSET);
        this->SSD1306_Cmd(0x00);
        this->SSD1306_Cmd(SSD1306_SETSTARTLINE);

        this->SSD1306_Cmd(SSD1306_CHARGEPUMP);
        this->SSD1306_Cmd(0x14);

        this->SSD1306_Cmd(SSD1306_MEMORYMODE); // 0x20
        this->SSD1306_Cmd(0x00); // 0x0 act like ks0108
        this->SSD1306_Cmd(SSD1306_SEGREMAP | 0x1);
        this->SSD1306_Cmd(SSD1306_COMSCANDEC);

        this->SSD1306_Cmd(SSD1306_SETCOMPINS);
        this->SSD1306_Cmd(0x12);
        this->SSD1306_Cmd(SSD1306_SETCONTRAST);
        this->SSD1306_Cmd(0xFF);

        this->SSD1306_Cmd(SSD1306_SETPRECHARGE); // 0xd9
        this->SSD1306_Cmd(0xF1);

        this->SSD1306_Cmd(SSD1306_SETVCOMDETECT); // 0xDB
        this->SSD1306_Cmd(0x40);
        this->SSD1306_Cmd(SSD1306_DISPLAYALLON_RESUME); // 0xA4
        this->SSD1306_Cmd(SSD1306_NORMALDISPLAY);       // 0xA6
        this->SSD1306_Cmd(SSD1306_DEACTIVATE_SCROLL);

        this->SSD1306_Cmd(SSD1306_DISPLAYON); // Main screen turn on
    }
    return ret;
}

void SSD1306::SSD1306_DrawPixel(int16_t x, int16_t y, uint8_t color) {
    if ((x >= 0) && (x < SSD1306_WIDTH) && (y >= 0) && (y < SSD1306_HEIGHT)) {
        
        switch (color) {
            case SSD1306_COLOR_WHITE:
                ssd1306Buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
            break;
            case SSD1306_COLOR_BLACK:
                ssd1306Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
            break;
            case SSD1306_COLOR_INVERSE:
                ssd1306Buffer[x + (y / 8) * SSD1306_WIDTH] ^= (1 << (y & 7));
            break;
            }
      }
}

void SSD1306::SSD1306_display() {
    this->SSD1306_Cmd(SSD1306_PAGEADDR);
    this->SSD1306_Cmd(0);                   // Page start address
    this->SSD1306_Cmd(7);                   // 0xff Page end (not really, but works here)
    this->SSD1306_Cmd(SSD1306_COLUMNADDR);
    this->SSD1306_Cmd(0);                   // Column start address
    this->SSD1306_Cmd(SSD1306_WIDTH - 1);   // Column end address
    
    this->SSD1306_Data(ssd1306Buffer, SSD1306_BUFFER_SIZE);
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
    return i2c_bus_transmit(this->i2c_ssd1306->dev_handle, cmdBuf, 2, 100);
}

ssd1306_err_t SSD1306::SSD1306_Data(uint8_t *buffer, size_t size) {
    uint8_t dataBuf[SSD1306_BUFFER_SIZE + 1];
    dataBuf[0] = SSD1306_DATA_ADDR;
    memcpy(&dataBuf[1], buffer, SSD1306_BUFFER_SIZE);
    return i2c_bus_transmit(this->i2c_ssd1306->dev_handle, dataBuf, SSD1306_BUFFER_SIZE + 1, 200);
}