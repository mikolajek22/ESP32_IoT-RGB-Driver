#include "oled_controller.h"
#include "oled_controller.hpp"
#include "esp_err.h"
#include "i2c_bus.h"

#define WHITE   SSD1306_COLOR_WHITE
#define BLACK   SSD1306_COLOR_BLACK

void oled_controller_main() {
    OLED_CONTROLLER controller;
    while(1) {
        controller.oled_controller_display();
        vTaskDelay(500);
    }
}

void OLED_CONTROLLER::oled_controller_display() {
    for (uint8_t i = 0; i < 9; i++) {
        switch(i){
          case 1:
          // ROUND RECTANGLE
            gfx->GFX_DrawRoundRect(50, 10, 40, 40, 5, WHITE);
            break;
          case 2:
            gfx->GFX_DrawRect(20, 10, 50, 20, WHITE);
            break;
          case 3:
            gfx->GFX_DrawCircle(60, 30, 15, WHITE);
            break;
          case 4:
            gfx->GFX_DrawFilledCircle(60, 30, 15, WHITE);
            break;
          case 5:
            gfx->GFX_DrawFilledRect(20, 10, 50, 20, WHITE);
            break;
          case 6:
            gfx->GFX_DrawFilledRoundRect(50, 10, 40, 40, 5, WHITE);
            break;
          case 7:
            gfx->GFX_DrawTriangle(5, 5, 60, 5, 30, 30, WHITE);
            break;  
          case 8:
            gfx->GFX_DrawFilledTriangle(5, 5, 60, 5, 30, 30, WHITE);
            break; 
        }
        ssd1306->SSD1306_display();
        vTaskDelay(1000);
        ssd1306->SSD1306_clear(BLACK);
      }
}