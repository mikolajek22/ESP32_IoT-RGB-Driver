
/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Mikolaj
 * @date           : 2025-07-04
 * @brief          : Main
 ******************************************************************************
 * Main app
 ******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_mac.h"
#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_http_server.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>
#include "fs.c"
#include "driver/ledc.h"
#include "http_server.h"

#include "http_server.h"

#include "startup.h"
#include "logs.h"
#include "sntp_sync.h"
#include "driver/timer.h"
#include "diagnostic.h"


#include "SSD1306.h"
#include "oled_controller.h"
#include "keyboard.h"
#include "led.h"

static const char *TAG = "MAIN";

TaskHandle_t taskRgbController          = NULL;
TaskHandle_t taskOledController         = NULL;
TaskHandle_t taskKeyboardController     = NULL;
TaskHandle_t taskLedController          = NULL;

QueueHandle_t timerQueue;
QueueHandle_t keyboardQueue;


extern void rgbController_main(void *pvParameters);
extern void rgbController_init();
extern void oledController_main(void);
bool IRAM_ATTR timer_callback(void);

void app_main()
{
    /* non-volotail flash initialization */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* little file system initialization */
    if (ESP_OK == fs_mount()) {
        ESP_LOGI(TAG, "LFS mounted successfully");

        /* Initialize Hardware */
        led_init();
        keyboard_init();
        rgbController_init();

        /* Read configuration file from file system */
        startup_ReadConfiguration();

        /* Obtain connection and set up a HTTP server */
        if (ESP_OK == http_server_init()) {
            if (ESP_OK == http_server_connect()) {
                /* synchronize time */
                sntp_sync_init();
                /* enable logging (UART / FILE / WEBSOCKET)*/
                logs_customizeLogs();
                /* diagnostic init */
                diagnostic_init();
            }
        }

        timerQueue = xQueueCreate(4, sizeof(int));
        keyboardQueue = xQueueCreate(4, sizeof(int));

        xTaskCreate(diagnostic_main, "timer_DiagnosticTask", 4096, NULL, 5, NULL);
        xTaskCreate(rgbController_main,"color_regulator", 8192, NULL, 2, &taskRgbController);
        xTaskCreate(oled_controller_main,"oledController_main", 4096, NULL, 5, &taskOledController);
        xTaskCreate(keyboard_main_task,"keyboard_main_task", 4096, NULL, 5, &taskKeyboardController);
        xTaskCreate(led_main_task,"led_main_task", 1024, NULL, 5, &taskLedController);
        
    } 
    else {
        ESP_LOGE(TAG, "Failed to mount LFS");
    }
}

    

