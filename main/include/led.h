#ifndef _LED_H
#define _LED_H
#include "main.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    LED_RUN = GPIO_NUM_4,
    LED_ERROR = GPIO_NUM_27,
} led_t;
void led_main_task(void);
void led_init();


void led_set(led_t led);

void led_reset(led_t led);

void led_toggle(led_t led);
#ifdef __cplusplus
}
#endif


#endif /* _LED_H */