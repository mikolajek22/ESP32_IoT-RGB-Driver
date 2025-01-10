#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/* queue for ISR interrupts */
extern QueueHandle_t timerQueue;
void diagnostic_main(void);