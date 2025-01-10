#include "diagnostic.h"
#include "esp_system.h"
#include "esp_heap_caps.h" 
#include "esp_log.h"
#include <inttypes.h>

/* diagnostic info */
typedef struct {
    float ramUsage;
    float maxRamUsage;
    uint32_t freeRam;
    uint32_t minRam;
    uint32_t uptime;
} diagnosticInfo_t;
diagnosticInfo_t diagnosticInfo;

void diagnostic_main(void) {
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