#ifndef _KEYBOARD_HPP
#define _KEYBOARD_HPP

#include "stdint.h"



// typedef struct {
//     uint8_t pin;
//     uint8_t state;
// } button_t;

class KEYBOARD {
    private:
    public:
        void keyboard_btn1_pressed();
        void keyboard_btn2_pressed();
        void keyboard_btn3_pressed();
        void keyboard_btn4_pressed();
        void keyboard_btn5_pressed();
};

#endif

