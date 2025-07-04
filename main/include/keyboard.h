#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif
    void keyboard_main_task(void);
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
#ifdef __cplusplus
}
#endif

#endif