#include "diagnostic.h"
#include "esp_system.h"
#include "esp_heap_caps.h" 
#include "esp_log.h"
#include <inttypes.h>
#include "driver/timer.h"

/* diagnostic info */
typedef struct {
    float ramUsage;
    float maxRamUsage;
    uint32_t freeRam;
    uint32_t minRam;
    uint32_t uptime;
} diagnosticInfo_t;
diagnosticInfo_t diagnosticInfo;

bool IRAM_ATTR timer_callback(void);

void diagnostic_init() {
    /* GPTimer initialization */
    timer_config_t timer_config = {
        .divider = 80,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);
    /* diagnostic status will be displayed every 5 seconds (in logs) */
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 5000000);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_callback, NULL, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

}

void diagnostic_main(void *pvParameters) {
    int signal;
    const char* diagnosticTag = "DIAGNOSTIC";
    while (1) {
        /* if received GPTimer interrupt */
        size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
        if (xQueueReceive(timerQueue, &signal, portMAX_DELAY)) {
            diagnosticInfo.freeRam = esp_get_free_heap_size();
            diagnosticInfo.minRam = esp_get_minimum_free_heap_size();
            diagnosticInfo.ramUsage = ((float)totalHeap - (float)diagnosticInfo.freeRam) / (float)totalHeap * 100;
            diagnosticInfo.maxRamUsage = ((float)totalHeap - (float)diagnosticInfo.minRam) / (float)totalHeap * 100;
            diagnosticInfo.uptime = esp_log_timestamp();
            // TODO: add flash usage
            ESP_LOGI(diagnosticTag, "system raport:\nRAM usage: %.2f percent\nMax RAM usage: %.2f percent\nFree RAM: %"PRIu32" bytes\nMin RAM: %"PRIu32" bytes\nRun Time: %"PRIu32" ms",
                diagnosticInfo.ramUsage,
                diagnosticInfo.maxRamUsage,
                diagnosticInfo.freeRam,
                diagnosticInfo.minRam,
                diagnosticInfo.uptime);
        }   
    }
}

    bool IRAM_ATTR timer_callback(void) {
        int signal = 1;
        xQueueSendFromISR(timerQueue, &signal, NULL);
        return true;
    }