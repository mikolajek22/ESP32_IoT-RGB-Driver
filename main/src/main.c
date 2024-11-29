#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include "esp_mac.h"
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
// #include "driver/adc.h"
#include "http_server.h"

#include "startup.h"

#define RED_LED_PIN         13
#define GREEN_LED_PIN       12
#define BLUE_LED_PIN        14

#define COLORS_AMOUNT       3
uint8_t const COLOR_PINS[3] = {RED_LED_PIN,GREEN_LED_PIN, BLUE_LED_PIN};
static const char *TAG = "MAIN";

static ledc_channel_config_t ledc_channel[3];
static ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_8_BIT,
    .freq_hz = 1000,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .clk_cfg = LEDC_AUTO_CLK,
};

/* struct that is passed to rgb controller */
typedef struct {
    ledc_channel_config_t *ledc_channel[3];
    ledc_timer_config_t *ledc_timer;
} rgbController_t;
static rgbController_t rgbParams;

TaskHandle_t taskRgbController  = NULL;
TaskHandle_t taskWebServer      = NULL;
extern void rgbController_main(void *pvParameters);

static void init_hw(void) {
    
    ledc_timer_config(&ledc_timer);
    for(int i=0;i<COLORS_AMOUNT;i++) {
        ledc_channel[i].channel = LEDC_CHANNEL_0+i;
        ledc_channel[i].duty = 0;
        ledc_channel[i].gpio_num = COLOR_PINS[i];
        ledc_channel[i].speed_mode = LEDC_HIGH_SPEED_MODE;
        ledc_channel[i].hpoint = 0;
        ledc_channel[i].timer_sel = LEDC_TIMER_0;
        ledc_channel_config(&ledc_channel[i]);
    }
    rgbParams.ledc_channel[0] = &ledc_channel[0];
    rgbParams.ledc_channel[1] = &ledc_channel[1];
    rgbParams.ledc_channel[2] = &ledc_channel[2];
    rgbParams.ledc_timer      = &ledc_timer;
}

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
    #define LEDC_MODE               LEDC_LOW_SPEED_MODE
    #define LEDC_DUTY               255
    #define LEDC_CHANNEL            LEDC_CHANNEL_0
    /* little file system initialization */
    if (ESP_OK == fs_mount()) {
        ESP_LOGI(TAG, "LFS mounted successfully");
        
        /* timers initialization */
        init_hw();
        startup_ReadConfiguration();
        /* simultanous tasks creation */
        xTaskCreatePinnedToCore(http_server_main,"http_server",16384,NULL,10,&taskWebServer,1);
        xTaskCreatePinnedToCore(rgbController_main,"color_regulator",16384, &rgbParams,5,&taskRgbController,1);
    } 
    else {
        ESP_LOGE(TAG, "Failed to mount LFS");
    }
    
}

    
    
    
    // setup_server(); 
    

    /*TODO:
    default gway to web site
    ram usage
    free memory
    flash usage
    files list on flash
    date and time? + actual time of working? maybe...*/