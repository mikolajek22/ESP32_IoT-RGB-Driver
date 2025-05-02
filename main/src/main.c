#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
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

#define RED_LED_PIN         GPIO_NUM_17
#define GREEN_LED_PIN       GPIO_NUM_18
#define BLUE_LED_PIN        GPIO_NUM_19

#define COLORS_AMOUNT       3



 /* ======================================== TYPEDEFS ======================================== */
typedef struct {
    ledc_channel_config_t *ledc_channel[3];
    ledc_timer_config_t *ledc_timer;
} rgbController_t;

 /* ======================================== CONSTS ======================================== */

uint8_t const COLOR_PINS[3] = {RED_LED_PIN, GREEN_LED_PIN, BLUE_LED_PIN};
static const char *TAG = "MAIN";

 /* ======================================== FREERTOS ======================================== */

TaskHandle_t taskRgbController  = NULL;
TaskHandle_t taskOledController = NULL;
TaskHandle_t taskKeyboardController = NULL;

QueueHandle_t timerQueue;
QueueHandle_t keyboardQueue;

 /* ======================================== VARS ======================================== */

static rgbController_t rgbParams;

 /* ======================================== PFP ======================================== */

extern void rgbController_main(void *pvParameters);
extern void oledController_main(void);
bool IRAM_ATTR timer_callback(void);
static void IRAM_ATTR btn_isr_iqr(void *arg);


static void init_hw(void) { 
    
    /* PWM CONFIG */
    static ledc_channel_config_t ledc_channel[3];
    static ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    
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

    /* BTN CONFIG */
    gpio_config_t btn_config = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN1_PIN) | (1ULL << BTN2_PIN) | (1ULL << BTN3_PIN) | (1ULL << BTN4_PIN) | (1ULL << BTN5_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&btn_config);

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        printf("gpio_install_isr_service failed: %s\n", esp_err_to_name(err));
    } // domyślny poziom priorytetu
    gpio_isr_handler_add(BTN1_PIN, btn_isr_iqr, (void*) BTN1_PIN);
    gpio_isr_handler_add(BTN2_PIN, btn_isr_iqr, (void*) BTN2_PIN);
    gpio_isr_handler_add(BTN3_PIN, btn_isr_iqr, (void*) BTN3_PIN);
    gpio_isr_handler_add(BTN4_PIN, btn_isr_iqr, (void*) BTN4_PIN);
    gpio_isr_handler_add(BTN5_PIN, btn_isr_iqr, (void*) BTN5_PIN);  
}

esp_err_t enableDiagnosticStatus(void) {
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
    return ESP_OK;
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
    
    /* little file system initialization */
    if (ESP_OK == fs_mount()) {
        ESP_LOGI(TAG, "LFS mounted successfully");
        /* Initialize LEDC timer to control RGB with PWM */
        init_hw();
        /* Read configuration file in order to set up static IP address, subnet mast etc. */
        startup_ReadConfiguration();

        /* Obtain connection and set up a HTTP server */
        if (ESP_OK == http_server_init()) {
            if (ESP_OK == http_server_connect()) {
                /* synchronize time */
                sntp_sync_init();
                /* enable logging (UART / FILE / WEBSOCKET)*/
                logs_customizeLogs();
                /* timers initialization to diagnostic*/
                enableDiagnosticStatus();
            }
        }
        // volatile oled_err_t oledStatus = 99;
        // oledStatus = oledController_init();

        // oledStatus = oledController_setUp();
        /* ==================================== TASKS ========================================== */
        /* RGB LED STRIP CONTROLLER */
        timerQueue = xQueueCreate(10, sizeof(int));
        keyboardQueue = xQueueCreate(1, sizeof(int));

        xTaskCreate(diagnostic_main, "timer_DiagnosticTask", 4096, NULL, 5, NULL);
        xTaskCreate(rgbController_main,"color_regulator", 8192, &rgbParams, 5, &taskRgbController);
        xTaskCreate(oled_controller_main,"oledController_main", 4096, NULL, 5, &taskOledController);
        xTaskCreate(keyboard_main_task,"keyboard_main_task", 4096, NULL, 5, &taskKeyboardController);
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

    /* ======================================== INTERRUPTS & CALLBACKS ======================================== */
   
    static void IRAM_ATTR btn_isr_iqr(void *arg) {
        uint32_t gpio_num = (uint32_t) arg;
        uint32_t gpioAction = 0;
        switch (gpio_num) {
            case BTN1_PIN:
                gpioAction = BTN_1_PRESSED;
                break;
            case BTN2_PIN:
                gpioAction = BTN_2_PRESSED;
                break;
            case BTN3_PIN:
                gpioAction = BTN_3_PRESSED;
                break;
            case BTN4_PIN:
                gpioAction = BTN_4_PRESSED;
                break;
            case BTN5_PIN:
                gpioAction = BTN_5_PRESSED;
                break;

        }
        xQueueSendFromISR(keyboardQueue, &gpioAction, NULL);
    }

    bool IRAM_ATTR timer_callback(void) {
        int signal = 1;
        xQueueSendFromISR(timerQueue, &signal, NULL);
        return true;
    }