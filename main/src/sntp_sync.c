#include "time.h"
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define CONFIG_SNTP_TIME_SERVER "pool.ntp.org"

static const char *TAG = "SNTP";
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void sntp_sync_ObtainActualTime(const char* bufTime){
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    char tempBufTime[64];
    strftime(tempBufTime, sizeof(tempBufTime), "%c", &timeinfo);
    sprintf(bufTime, tempBufTime);
}

 void sntp_sync_init()  {
    char timeBuffer[60];
    ESP_LOGI(TAG, "Initializing and starting SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
    config.sync_cb = time_sync_notification_cb;
    esp_netif_sntp_init(&config);
    // struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    
    // time_t now = 0;
    // time(&now);
    // localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    sntp_sync_ObtainActualTime(strftime_buf);
    // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	ESP_LOGI(TAG, "The current date/time in Poland is: %s", strftime_buf);
}