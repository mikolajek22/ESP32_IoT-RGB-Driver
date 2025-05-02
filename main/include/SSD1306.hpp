#ifndef _SDD1306_HPP
#define _SDD1306_HPP

#include "stdint.h"
#include "esp_err.h"
#include "i2c_bus.h"

#define ssd1306_err_t  esp_err_t
#define SSD1306_OK     ESP_OK
#define SSD1306_ERR    ESP_FAIL

#define SSD1306_ADDR               0x3C
#define SSD1306_HEIGHT             64
#define SSD1306_WIDTH              128

#define SSD1306_BUFFER_SIZE         SSD1306_WIDTH * SSD1306_HEIGHT / 8

#define SSD1306_CMD_ADDR           0x00
#define SSD1306_DATA_ADDR          0x40

#define SSD1306_COLOR_WHITE         0
#define SSD1306_COLOR_BLACK         1
#define SSD1306_COLOR_INVERSE       2

// ======================  REGISTERS  ========================= //
#define SSD1306_MEMORYMODE             0x20           
#define SSD1306_COLUMNADDR             0x21           
#define SSD1306_PAGEADDR               0x22             
#define SSD1306_SETCONTRAST            0x81          
#define SSD1306_CHARGEPUMP             0x8D           
#define SSD1306_SEGREMAP               0xA0             
#define SSD1306_DISPLAYALLON_RESUME    0xA4  
#define SSD1306_DISPLAYALLON           0xA5         
#define SSD1306_NORMALDISPLAY          0xA6        
#define SSD1306_INVERTDISPLAY          0xA7        
#define SSD1306_SETMULTIPLEX           0xA8         
#define SSD1306_DISPLAYOFF             0xAE           
#define SSD1306_DISPLAYON              0xAF            
#define SSD1306_COMSCANINC             0xC0           
#define SSD1306_COMSCANDEC             0xC8           
#define SSD1306_SETDISPLAYOFFSET       0xD3     
#define SSD1306_SETDISPLAYCLOCKDIV     0xD5   
#define SSD1306_SETPRECHARGE           0xD9         
#define SSD1306_SETCOMPINS             0xDA           
#define SSD1306_SETVCOMDETECT          0xDB        

#define SSD1306_SETLOWCOLUMN           0x00   
#define SSD1306_SETHIGHCOLUMN          0x10  
#define SSD1306_SETSTARTLINE           0x40   

#define SSD1306_EXTERNALVCC            0x01
#define SSD1306_SWITCHCAPVCC           0x02
#define SSD1306_DEACTIVATE_SCROLL      0x2E 



class SSD1306 {
    private:
        i2c_bus_device_t *i2c_ssd1306;
        static uint8_t ssd1306Buffer[SSD1306_BUFFER_SIZE];
        ssd1306_err_t SSD1306_Cmd(uint8_t cmd);
        ssd1306_err_t SSD1306_Data(uint8_t *buffer, size_t size);

    public:

        SSD1306(i2c_bus_device_t *i2c_ssd1306) {
            this->i2c_ssd1306 = i2c_ssd1306;
        }
        ssd1306_err_t SSD1306_init();
        void SSD1306_DrawPixel(int16_t x, int16_t y, uint8_t color);
        void SSD1306_display();
        void SSD1306_clear(uint8_t color);

};

#endif