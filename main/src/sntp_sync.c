#include "time.h"
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sntp_sync.h"

#define CONFIG_SNTP_TIME_SERVER     "pool.ntp.org"
#define MAX_TIME_BUFFER_LEN         64
#define MAX_SYNC_RETRIES            10

#define DELAY_MS                    2000 / portTICK_PERIOD_MS // (2000 ms)

static const char *TAG = "SNTP";

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "SNTP Synchronized");
}

esp_err_t sntp_sync_ObtainActualTime(const char* bufTime){
    time_t now = 0;
    struct tm timeinfo = { 0 };
    char tempBufTime[MAX_TIME_BUFFER_LEN];

    time(&now);
    localtime_r(&now, &timeinfo);

    if (0 >= strftime(tempBufTime, sizeof(tempBufTime), "%d/%m/%Y %H:%M:%S", &timeinfo)) {
        return ESP_FAIL;
    }
    if (0 >= sprintf(bufTime, tempBufTime)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

 esp_err_t sntp_sync_init()  {
    char timeBuffer[MAX_TIME_BUFFER_LEN];
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
    config.sync_cb = time_sync_notification_cb;

    if (ESP_OK == esp_netif_sntp_init(&config)) {
        int retry = 0;
        const int retry_count = MAX_SYNC_RETRIES;
        while (esp_netif_sntp_sync_wait(DELAY_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
            ESP_LOGI(TAG, "SNTP time sync retry... (%d/%d)", retry, retry_count);
        }
        if (retry == retry_count) {
            ESP_LOGI(TAG, "Failed to synchronize with SNTP");
            return ESP_FAIL;
        }
        else {
            setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
            tzset();
            char strftime_buf[MAX_TIME_BUFFER_LEN];
            if (ESP_OK != sntp_sync_ObtainActualTime(strftime_buf)) {
                ESP_LOGI(TAG, "Could not obtain actual time");
                return ESP_FAIL;
            }
            else {
                ESP_LOGI(TAG, "The current date & time in Poland is: %s\n", strftime_buf);
                return ESP_OK;
            }
        }
    }
    else {
        return ESP_FAIL;
    }
}