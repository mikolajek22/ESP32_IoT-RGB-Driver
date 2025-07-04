#ifndef _MAIN_H
#define _MAIN_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern QueueHandle_t keyboardQueue;
extern QueueHandle_t timerQueue;

#endif /* _MAIN_H */