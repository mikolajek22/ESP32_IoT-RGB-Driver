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

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "SNTP Synchronized");
}

void sntp_sync_ObtainActualTime(const char* bufTime){
    time_t now = 0;
    struct tm timeinfo = { 0 };
    char tempBufTime[64];

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(tempBufTime, sizeof(tempBufTime), "%d/%m/%Y %H:%M:%S", &timeinfo);
    sprintf(bufTime, tempBufTime);
}

 void sntp_sync_init()  {
    char timeBuffer[64];
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
    config.sync_cb = time_sync_notification_cb;

    ESP_LOGI(TAG, "Initializing and starting SNTP");
    esp_netif_sntp_init(&config);

    int retry = 0;
    const int retry_count = 10;

    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "SNTP time sync retry... (%d/%d)", retry, retry_count);
    }
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    
    char strftime_buf[64];
    sntp_sync_ObtainActualTime(strftime_buf);
	ESP_LOGI(TAG, "The current date & time in Poland is: %s\n", strftime_buf);
}