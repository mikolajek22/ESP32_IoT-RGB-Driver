#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include "sntp_sync.h"
#include "http_server.h"
#include "http_handlers.h"
#include "fs.h"

#define MAX_TIME_BUFFER_LEN     64
#define MAX_LOG_BUFFER_LEN      MAX_TIME_BUFFER_LEN + 256
#define MAX_LOGS_FILE_SIZE      512*1024
#define LOGS_FILE_NAME          "logs.txt"

void deleteAnsiTrash(char *log) {
    char* src = log;
    char* dst = log;
    /* e.g. 13/12/2024 10:54:49 - \033[0;33mW (10:54:49.876) RGB: written 0, 0, 0\033[0m 
    *  \033 _ ASNI END shoul be deleted from char sequence till the 'm' char.
    */
    while(*src) {
        if (*src == '\033') {
            while (*src && *src != 'm') {
                src++;
            }
            if (*src) {
                src++;
            }
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

int vprintf_custom(const char* fmt, va_list args){

    char timeStamp[MAX_TIME_BUFFER_LEN] = {0};
    sntp_sync_ObtainActualTime(timeStamp);

    char fixedLog[MAX_LOG_BUFFER_LEN]   = {0};
    // add actual date/time to the log.
    snprintf(fixedLog, sizeof(fixedLog), "%s - %s", timeStamp, fmt);

    bool WS_LOGS_ENABLE = true;
    bool FS_LOGS_ENABLE = true;
    bool UART_LOGS_ENABLE = true;
    if (WS_LOGS_ENABLE || FS_LOGS_ENABLE) {
        // prepare frame to send.
        char fixedLogWS[MAX_LOG_BUFFER_LEN] = {0};
        vsprintf(fixedLogWS, fixedLog, args);
        // remove color info.
        deleteAnsiTrash(fixedLogWS);
        if (WS_LOGS_ENABLE) {
            // send log over WS.
            http_handlers_sendOverWS(fixedLogWS);
        }
        if (FS_LOGS_ENABLE) {
            // write log into FS
            deleteAnsiTrash(fixedLogWS);
            uint8_t fileID;
            if (ESP_OK == fs_findID(&fileID)) {
                if (ESP_OK == fs_openFile(fileID, LOGS_FILE_NAME, APPEND_PERMISSION)) {
                    if (MAX_LOGS_FILE_SIZE <= fs_fileSize(fileID)) {
                        fs_rewindFile(fileID, false);
                    }
                    fs_writeFile(fileID, LOGS_FILE_NAME, fixedLogWS, strlen(fixedLogWS));
                    fs_closeFile(fileID);
                }
            }  
        }
    }
    // send log over UART
    return UART_LOGS_ENABLE ? vprintf(fixedLog, args) : 0;
}

void logs_customizeLogs(){
    esp_log_set_vprintf(vprintf_custom);
}

