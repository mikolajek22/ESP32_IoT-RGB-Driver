#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include "sntp_sync.h"

int vprintf_custom(const char* fmt, va_list args){
    char timeStamp[64] = {0};
    char fixedLog[256+64] = {0};
    sntp_sync_ObtainActualTime(timeStamp);
    sprintf(fixedLog, timeStamp);
    // sprintf(fixedLog + sizeof(timeStamp), fmt);
    printf("%s", timeStamp);
    return vprintf(fmt, args);
    
}

void logs_customizeLogs(){
    printf("logs customize");
    esp_log_set_vprintf(vprintf_custom);
}
