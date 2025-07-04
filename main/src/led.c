#include "led.h"


void led_main_task(void *pvParameters) {
    while (1) {
        // led_set(LED_RUN);
        led_reset(LED_RUN);
        vTaskDelay(pdMS_TO_TICKS(1000));
        led_set(LED_RUN);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void led_init() {
    gpio_config_t led_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_RUN) | (1ULL << LED_ERROR),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&led_config);

    led_reset(LED_RUN);
    led_reset(LED_ERROR);
}

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