#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif
    
#define BTN1_PIN            GPIO_NUM_23 
#define BTN2_PIN            GPIO_NUM_25
#define BTN3_PIN            GPIO_NUM_26
#define BTN4_PIN            GPIO_NUM_5
#define BTN5_PIN            GPIO_NUM_16

typedef enum {
    BTN_1_PRESSED   = 0,
    BTN_2_PRESSED,
    BTN_3_PRESSED,
    BTN_4_PRESSED,
    BTN_5_PRESSED,
    BTN_1_RELEASED,
    BTN_2_RELEASED,
    BTN_3_RELEASED,
    BTN_4_RELEASED,
    BTN_5_RELEASED,
} btn_Status_t;

void keyboard_main_task(void *pvParameters);
void keyboard_init();

#ifdef __cplusplus
}
#endif

#endif /* _KEYBOARD_H */