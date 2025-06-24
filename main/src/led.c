#include "led.h"


void led_main_task(void) {
    while (1) {
        // led_set(LED_RUN);
        led_reset(LED_ERROR);
        vTaskDelay(pdMS_TO_TICKS(500));
        led_reset(LED_RUN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void led_init();

void led_set(led_t led) {
    gpio_set_level((gpio_num_t)led, 1);
}

void led_reset(led_t led) {
    gpio_set_level((gpio_num_t)led, 0);
}

void led_toggle(led_t led) {
    if (gpio_get_level((gpio_num_t)led)) {
        gpio_set_level((gpio_num_t)led, 0);
    }
    else {
        gpio_set_level((gpio_num_t)led, 1);
    }
}