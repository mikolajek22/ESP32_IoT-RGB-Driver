#include "oled_controller.h"
#include "oled_controller.hpp"
#include "esp_err.h"
#include "i2c_bus.h"



#define WHITE   SSD1306_COLOR_WHITE
#define BLACK   SSD1306_COLOR_BLACK
#define MAX_TIME_BUFFER_LEN 64


#define LINE_Y_OFFSET       5
#define LINE_X_OFFSET       5
#define MENU_X_OFFSET       15
#define LINE_Y_DIFF         15
#define MENU_Y_DIFF         10

#define MAX_DATE_STR_LEN    11
#define MAX_TIME_STR_LEN    9
#define MAX_IP_STR_LEN      16


menu_t menu;
OLED_CONTROLLER *controller;
uint8_t dispIpAddr[4];
void oled_controller_main() {
    controller = new OLED_CONTROLLER();
    menu.actualPage = TIME_PAGE;
    while(1) {
        controller->oled_controller_display();
        vTaskDelay(pdMS_TO_TICKS(100));  //refersh each second.
    }
}

void OLED_CONTROLLER::oled_controller_display() {
  ssd1306->SSD1306_clear(BLACK);
  switch (menu.actualPage) {
    case SAVER_PAGE:
    break;
    case TIME_PAGE:
      this->oled_controller_time_page();
    break;
    case MENU_PAGE:
      this->oled_controller_main_menu();
    break;
      
    case NETWORK_PAGE:
      this->oled_controller_network_menu();
    break;

    case IP_ADDR_PAGE:
      this->oled_controller_ipAddr_page();
    break;

    case REBOOT_PAGE:
      this->oled_controller_reboot_page();
    break;
    default:
    break;
  }
  ssd1306->SSD1306_display();
}

void OLED_CONTROLLER::oled_controller_time_page() {
  char timeStamp[MAX_TIME_BUFFER_LEN] = {0};
  char date[MAX_DATE_STR_LEN]         = {0};
  char time[MAX_TIME_STR_LEN]         = {0};

  sntp_sync_ObtainActualTime(timeStamp);
  snprintf(date, MAX_DATE_STR_LEN, timeStamp);
  snprintf(time, MAX_TIME_STR_LEN, timeStamp + MAX_DATE_STR_LEN);

  this->oled_controller_drawFrame();

  gfx->GFX_DrawString(LINE_X_OFFSET, LINE_Y_OFFSET,                         "IOT RGB Driver", WHITE, BLACK);
  gfx->GFX_DrawString(LINE_X_OFFSET, LINE_Y_OFFSET + LINE_Y_DIFF,           "date:", WHITE, BLACK);
  gfx->GFX_DrawString(LINE_X_OFFSET, LINE_Y_OFFSET + 2 * LINE_Y_DIFF,       "time:", WHITE, BLACK);
  gfx->GFX_DrawString(12 * LINE_X_OFFSET, LINE_Y_OFFSET + LINE_Y_DIFF,      date, WHITE, BLACK);
  gfx->GFX_DrawString(12 * LINE_X_OFFSET, LINE_Y_OFFSET + 2 * LINE_Y_DIFF,  time, WHITE, BLACK);
}

void OLED_CONTROLLER::oled_controller_main_menu() {
  char *menuTable[] = {
    "network",
    "reboot",
    "back",
  };

  const uint8_t MAX_CELL_POINTER = 2;
  const uint8_t MIN_CELL_POINTER = 0;

  /* LIMITS - DO IT BETTER */
  if (menu.cellPointer < MIN_CELL_POINTER) menu.cellPointer = MAX_CELL_POINTER;
  if (menu.cellPointer > MAX_CELL_POINTER) menu.cellPointer = MIN_CELL_POINTER;

  this->oled_controller_drawFrame();
  this->oled_triangleCell(menu.cellPointer);
  uint8_t lineIdx = 0;

  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET, menuTable[lineIdx++], WHITE, BLACK);
  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET + MENU_Y_DIFF, menuTable[lineIdx++], WHITE, BLACK);
  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET + 2 * MENU_Y_DIFF, menuTable[lineIdx++], WHITE, BLACK);

  return;
}

void OLED_CONTROLLER::oled_controller_network_menu() {
    char *menuTable[] = {
    "ip addr",
    "netmask",
    "back",
  };

  const uint8_t MAX_CELL_POINTER = 2;
  const uint8_t MIN_CELL_POINTER = 0;

  /* LIMITS - DO IT BETTER */
  if (menu.cellPointer < MIN_CELL_POINTER) menu.cellPointer = MAX_CELL_POINTER;
  if (menu.cellPointer > MAX_CELL_POINTER) menu.cellPointer = MIN_CELL_POINTER;

  this->oled_controller_drawFrame();
  this->oled_triangleCell(menu.cellPointer);
  uint8_t lineIdx = 0;

  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET, menuTable[lineIdx++], WHITE, BLACK);
  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET + MENU_Y_DIFF, menuTable[lineIdx++], WHITE, BLACK);
  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET + 2 * MENU_Y_DIFF, menuTable[lineIdx++], WHITE, BLACK);
  return;
}

void OLED_CONTROLLER::oled_controller_saver_page() {
  
  return;
}


void OLED_CONTROLLER::oled_controller_ipAddr_page() {
  char ipAddr[MAX_IP_STR_LEN];

  /* LIMITS - DO IT BETTER */
  
  this->oled_controller_drawFrame();
  this->oled_triangleCell(2);

  snprintf(ipAddr, MAX_IP_STR_LEN, "%d.%d.%d.%d", dispIpAddr[0], dispIpAddr[1], dispIpAddr[2], dispIpAddr[3]);

  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET, "ip address:", WHITE, BLACK);
  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET + MENU_Y_DIFF, ipAddr, WHITE, BLACK);
  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET + 2 * MENU_Y_DIFF, "back", WHITE, BLACK);
  return;
}


void OLED_CONTROLLER::oled_controller_reboot_page() {
  static uint8_t leftToReboot = 3;
  char leftToRebootSec[2];

  this->oled_controller_drawFrame();
  snprintf(leftToRebootSec, 8, "%d", leftToReboot);
  gfx->GFX_DrawString(MENU_X_OFFSET, LINE_Y_OFFSET, "Rebooting ...", WHITE, BLACK);

  gfx->GFX_DrawString(60, 31, leftToRebootSec, WHITE, BLACK);
  gfx->GFX_DrawCircle(62, 35, 10, WHITE);

  vTaskDelay(pdMS_TO_TICKS(900));
  if (leftToReboot == 0) {
    esp_restart();
  }
  leftToReboot--;
}



void OLED_CONTROLLER::oled_accept_cell() {
  if (menu.actualPage == MENU_PAGE) {
    switch (menu.cellPointer) {
      case 0:
        menu.actualPage = NETWORK_PAGE;
      break;
      case 1:
        menu.actualPage = REBOOT_PAGE;
      break;
      case 2:
        menu.actualPage = TIME_PAGE;
      break;
    }
  }
  else if (menu.actualPage == NETWORK_PAGE) {
    switch (menu.cellPointer) {
      case 0:
        dispIpAddr[0] = 192;
        dispIpAddr[1] = 168;
        dispIpAddr[2] = 0;
        dispIpAddr[3] = 99;
        menu.actualPage = IP_ADDR_PAGE;
      break;
      case 1:
        dispIpAddr[0] = 255;
        dispIpAddr[1] = 255;
        dispIpAddr[2] = 255;
        dispIpAddr[3] = 0;
        menu.actualPage = IP_ADDR_PAGE;
      break;
      case 2:
        menu.actualPage = TIME_PAGE;
      break;
    }
  }

  else if (menu.actualPage == IP_ADDR_PAGE) {
     menu.actualPage = TIME_PAGE;
  }
  else {
    menu.actualPage = TIME_PAGE;
  }


  menu.cellPointer = 0; 
  return;
}
void OLED_CONTROLLER::oled_next_cell() {
  menu.cellPointer++;
}
void OLED_CONTROLLER::oled_prev_cell() {
  menu.cellPointer--;
}
void OLED_CONTROLLER::oled_triangleCell(uint8_t cell) {
  static const uint8_t firstXY[6] = {5, 12, 5, 6, 10, 9};
  if (cell < 5) {
    gfx->GFX_DrawFilledTriangle(firstXY[0], firstXY[1] + 10 * cell, firstXY[2], firstXY[3] + 10 * cell, firstXY[4], firstXY[5] + 10 * cell, WHITE);
  }
}
void OLED_CONTROLLER::oled_controller_drawFrame() {
    gfx->GFX_DrawRect(2, 2, 124, 60, WHITE);
    return;
}