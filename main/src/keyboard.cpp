#include "keyboard.hpp"
#include "keyboard.h"
#include "main.h"
#include "esp_log.h"
#include "driver/gpio.h"
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
                default:
                    ESP_LOGE(TAG, "Undefined Value");
                break;
            }
        }
    }
}

void KEYBOARD::keyboard_btn1_pressed() {
    ESP_LOGE(TAG, "BTN1 Pressed!");
    return;
}
void KEYBOARD::keyboard_btn2_pressed() {
    ESP_LOGE(TAG, "BTN2 Pressed!");
    return;
}
void KEYBOARD::keyboard_btn3_pressed() {
    ESP_LOGE(TAG, "BTN3 Pressed!");
    return;
}
void KEYBOARD::keyboard_btn4_pressed() {
    ESP_LOGE(TAG, "BTN4 Pressed!");
    return;
}
void KEYBOARD::keyboard_btn5_pressed() {
    ESP_LOGE(TAG, "BTN5 Pressed!");
    return;
}