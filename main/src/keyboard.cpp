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

static void IRAM_ATTR btn_isr_iqr(void *arg);

void keyboard_init() {
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << BTN1_PIN) | (1ULL << BTN2_PIN) | (1ULL << BTN3_PIN) | (1ULL << BTN4_PIN) | (1ULL << BTN5_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&btn_config);

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s\n", esp_err_to_name(err));
    }
    gpio_isr_handler_add(BTN1_PIN, btn_isr_iqr, (void*) BTN1_PIN);
    gpio_isr_handler_add(BTN2_PIN, btn_isr_iqr, (void*) BTN2_PIN);
    gpio_isr_handler_add(BTN3_PIN, btn_isr_iqr, (void*) BTN3_PIN);
    gpio_isr_handler_add(BTN4_PIN, btn_isr_iqr, (void*) BTN4_PIN);
    gpio_isr_handler_add(BTN5_PIN, btn_isr_iqr, (void*) BTN5_PIN); 
}

void keyboard_main_task() {
    while(1) {
        int btnNo;
        TickType_t nowTick              = 0;
        TickType_t prevTick             = 0;
        const TickType_t debouncingTick = pdMS_TO_TICKS(50);
        /* Debounce keyboard (50 ms) */
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
    
    ESP_LOGI(TAG, "BTN1 Pressed!");
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
    ESP_LOGI(TAG, "BTN2 Pressed!");
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
    ESP_LOGI(TAG, "BTN3 Pressed!");
    
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
    ESP_LOGI(TAG, "BTN4 Pressed!");
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
    ESP_LOGI(TAG, "BTN5 Pressed!");
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


 static void IRAM_ATTR btn_isr_iqr(void *arg) {
        uint32_t gpio_num = (uint32_t) arg;
        uint32_t gpioAction = 0;
        switch (gpio_num) {
            case BTN1_PIN:
                if (!gpio_get_level(BTN1_PIN)) {
                    gpioAction = BTN_1_PRESSED;
                }
                else {
                    gpioAction = BTN_1_RELEASED;
                }
                
                break;
            case BTN2_PIN:
                if (!gpio_get_level(BTN2_PIN)) {
                    gpioAction = BTN_2_PRESSED;
                }
                else {
                    gpioAction = BTN_2_RELEASED;
                }
                // gpioAction = BTN_2_PRESSED;
                break;
            case BTN3_PIN:
                if (!gpio_get_level(BTN3_PIN)) {
                    gpioAction = BTN_3_PRESSED;
                }
                else {
                    gpioAction = BTN_3_RELEASED;
                }
                // gpioAction = BTN_3_PRESSED;
                break;
            case BTN4_PIN:
                if (!gpio_get_level(BTN4_PIN)) {
                    gpioAction = BTN_4_PRESSED;
                }
                else {
                    gpioAction = BTN_4_RELEASED;
                }
                // gpioAction = BTN_4_PRESSED;
                break;
            case BTN5_PIN:
                if (!gpio_get_level(BTN5_PIN)) {
                    gpioAction = BTN_5_PRESSED;
                }
                else {
                    gpioAction = BTN_5_RELEASED;
                }
                // gpioAction = BTN_5_PRESSED;
                break;
            // case BTN1_PIN:
            //     gpioAction = BTN_1_PRESSED;
            //     break;
            // case BTN2_PIN:
            //     gpioAction = BTN_2_PRESSED;
            //     break;
            // case BTN3_PIN:
            //     gpioAction = BTN_3_PRESSED;
            //     break;
            // case BTN4_PIN:
            //     gpioAction = BTN_4_PRESSED;
            //     break;
            // case BTN5_PIN:
            //     gpioAction = BTN_5_PRESSED;
            //     break;

        }
        xQueueSendFromISR(keyboardQueue, &gpioAction, NULL);
    }