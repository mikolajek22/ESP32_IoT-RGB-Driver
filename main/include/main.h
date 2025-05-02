#ifndef _MAIN_H
#define _MAIN_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define BTN1_PIN            GPIO_NUM_23 
#define BTN2_PIN            GPIO_NUM_25
#define BTN3_PIN            GPIO_NUM_26
#define BTN4_PIN            GPIO_NUM_5
#define BTN5_PIN            GPIO_NUM_16

extern QueueHandle_t keyboardQueue;
#endif