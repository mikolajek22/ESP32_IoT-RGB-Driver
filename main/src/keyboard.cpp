#include "keyboard.hpp"
#include "keyboard.h"
#include "main.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "oled_controller.hpp"
#include "led.h"
KEYBOARD keyboard;
extern QueueHandle_t keyboardQueue;
static const char *TAG = "KEYBOARD";
void keyboard_main_task() {
    while(1) {
        int btnNo;
        TickType_t nowTick = 0;
        TickType_t prevTick = 0;
        const TickType_t debouncingTick = pdMS_TO_TICKS(50);
        if (xQueueReceive(keyboardQueue, &btnNo, portMAX_DELAY)) {
            prevTick = xTaskGetTickCount();
            switch (btnNo) {
                case BTN_1_PRESSED:
                    while (!gpio_get_level(BTN1_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn1_pressed();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_2_PRESSED:
                    while (!gpio_get_level(BTN2_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn2_pressed();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_3_PRESSED:
                    while (!gpio_get_level(BTN3_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn3_pressed();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_4_PRESSED:
                    while (!gpio_get_level(BTN4_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn4_pressed();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_5_PRESSED:
                    while (!gpio_get_level(BTN5_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn5_pressed();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                    break;

                    case BTN_1_RELEASED:
                    while (gpio_get_level(BTN1_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn1_released();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_2_RELEASED:
                    while (gpio_get_level(BTN2_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn2_released();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_3_RELEASED:
                    while (gpio_get_level(BTN3_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn3_released();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_4_RELEASED:
                    while (gpio_get_level(BTN4_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn4_released();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                break;
                case BTN_5_RELEASED:
                    while (gpio_get_level(BTN5_PIN)) {
                        nowTick = xTaskGetTickCount();
                        if (nowTick - prevTick > debouncingTick) {
                            keyboard.keyboard_btn5_released();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                    break;
                default:
                    ESP_LOGE(TAG, "Undefined Value");
                break;
            }
        }
    }
}

/* UP*/
void KEYBOARD::keyboard_btn1_pressed() {
    led_set(LED_ERROR);
    if (menu.actualPage == TIME_PAGE || menu.actualPage == NETWORK_PAGE) {
        menu.actualPage = MENU_PAGE;
    }
    else if (menu.actualPage == MENU_PAGE){
        controller->oled_prev_cell();
    }
    
    ESP_LOGE(TAG, "BTN1 Pressed!");
    return;
}

/* MIDDLE */
void KEYBOARD::keyboard_btn2_pressed() {
    led_set(LED_ERROR);
    if (menu.actualPage == TIME_PAGE) {
        menu.actualPage = MENU_PAGE;
    }
    else if (menu.actualPage == MENU_PAGE || menu.actualPage == NETWORK_PAGE || menu.actualPage == IP_ADDR_PAGE){
        controller->oled_accept_cell();
    }
    ESP_LOGE(TAG, "BTN2 Pressed!");
    return;
}

/* DOWN */
void KEYBOARD::keyboard_btn3_pressed() {
    led_set(LED_ERROR);
    if (menu.actualPage == TIME_PAGE) {
        menu.actualPage = MENU_PAGE;
    }
    else if (menu.actualPage == MENU_PAGE || menu.actualPage == NETWORK_PAGE){
        controller->oled_next_cell();
    }
    ESP_LOGE(TAG, "BTN3 Pressed!");
    
    return;
}

/* LEFT */
void KEYBOARD::keyboard_btn4_pressed() {
    led_set(LED_ERROR);
    if (menu.actualPage == TIME_PAGE) {
        menu.actualPage = MENU_PAGE;
    }
    else if (menu.actualPage == MENU_PAGE){
        
    }
    ESP_LOGE(TAG, "BTN4 Pressed!");
    return;
}

/* RIGHT */
void KEYBOARD::keyboard_btn5_pressed() {
    led_set(LED_ERROR);
    if (menu.actualPage == TIME_PAGE) {
        menu.actualPage = MENU_PAGE;
    }
    else if (menu.actualPage == MENU_PAGE){
        
    }
    ESP_LOGE(TAG, "BTN5 Pressed!");
    return;
}

void KEYBOARD::keyboard_btn1_released() {
    led_reset(LED_ERROR);
}
void KEYBOARD::keyboard_btn2_released() {
    led_reset(LED_ERROR);
}
void KEYBOARD::keyboard_btn3_released() {
    led_reset(LED_ERROR);
}
void KEYBOARD::keyboard_btn4_released() {
    led_reset(LED_ERROR);
}
void KEYBOARD::keyboard_btn5_released() {
    led_reset(LED_ERROR);
}