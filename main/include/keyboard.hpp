#ifndef _KEYBOARD_HPP
#define _KEYBOARD_HPP

#include "stdint.h"

#define BTN1_PIN            GPIO_NUM_23 
#define BTN2_PIN            GPIO_NUM_25
#define BTN3_PIN            GPIO_NUM_26
#define BTN4_PIN            GPIO_NUM_5
#define BTN5_PIN            GPIO_NUM_16

class KEYBOARD {
    private:
    public:
        void keyboard_btn1_pressed();
        void keyboard_btn2_pressed();
        void keyboard_btn3_pressed();
        void keyboard_btn4_pressed();
        void keyboard_btn5_pressed();

        void keyboard_btn1_released();
        void keyboard_btn2_released();
        void keyboard_btn3_released();
        void keyboard_btn4_released();
        void keyboard_btn5_released();
};

#endif

