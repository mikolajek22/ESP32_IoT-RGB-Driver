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
    } btn_Status_t;
#ifdef __cplusplus
}
#endif

#endif