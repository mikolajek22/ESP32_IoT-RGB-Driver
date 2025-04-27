#ifndef _OLED_CONTROLLER_HPP
#define _OLED_CONTROLLER_HPP

#include "gfx.hpp"
#include "SSD1306.h"
// #include "i2c_bus.h"

static i2c_bus_device_t i2c_ssd1306 = {
    .dev_cfg = {
        .dev_addr_length    = I2C_ADDR_BIT_LEN_7,
        .device_address    = SSD1306_ADDR,
        .scl_speed_hz       = 100000,
    },
};

class OLED_CONTROLLER {
    public:
        void oled_controller_display();
        OLED_CONTROLLER() {
            ssd1306 = new SSD1306(&i2c_ssd1306);
            gfx = new GFX(ssd1306);
            ssd1306->SSD1306_init();
        }

       

        ~OLED_CONTROLLER() {
            delete gfx;
            delete ssd1306;
        }
    private:
        GFX *gfx = nullptr;
        SSD1306 *ssd1306 = nullptr;
        
};

#endif