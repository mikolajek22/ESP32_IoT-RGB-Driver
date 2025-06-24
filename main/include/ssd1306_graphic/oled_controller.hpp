#ifndef _OLED_CONTROLLER_HPP
#define _OLED_CONTROLLER_HPP

#include "gfx.hpp"
#include "SSD1306.h"
// #include "i2c_bus.h"
#include "fonts.h"


typedef enum {
    SAVER_PAGE = 0,
    TIME_PAGE,
    MENU_PAGE,
    NETWORK_PAGE,
    IP_ADDR_PAGE,
    NETMASK_PAGE,
    REBOOT_PAGE,
} manu_page_t;

typedef struct {
    manu_page_t actualPage;
    int8_t cellPointer = 0;            // what does the arrow show rn?
} menu_t;
extern menu_t menu;

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
            gfx->GFX_SetFont(font_8x5);
            gfx->GFX_SetFontSize(1);
        }

       
        void oled_controller_main_menu();
        void oled_controller_network_menu();
        void oled_controller_time_page();
        void oled_controller_saver_page();
        void oled_controller_ipAddr_page();
        void oled_controller_reboot_page();

        void oled_accept_cell();
        void oled_next_cell();
        void oled_prev_cell();

        void oled_controller_drawFrame();
        ~OLED_CONTROLLER() {
            delete gfx;
            delete ssd1306;
        }
    private:
        GFX *gfx = nullptr;
        SSD1306 *ssd1306 = nullptr;

        void oled_triangleCell(uint8_t cell);
        
};

extern OLED_CONTROLLER *controller;

#endif