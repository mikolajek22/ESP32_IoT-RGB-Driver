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

#define RED_LED_PIN 13
#define GREEN_LED_PIN 12
#define BLUE_LED_PIN 14

#define SAMPLE_CNT 32

static ledc_channel_config_t ledc_channel[3];
int COLOR_AMOUNT = 3;
int COLOR_PINS[3]={RED_LED_PIN,GREEN_LED_PIN, BLUE_LED_PIN};
bool sequence_status;
int sequence_no;
bool breakFlag;


static const char *TAG = "MAIN"; // TAG for debug

int r_value=0, g_value=0, b_value=0;
int *pr_value=&r_value, *pg_value=&g_value, *pb_value=&b_value; //pointery do wartości

TaskHandle_t myTaskHandle = NULL;
TaskHandle_t myTaskHandle2 = NULL;
TaskHandle_t myTaskHandleHTTP = NULL;

esp_err_t change_color_sequence(httpd_req_t *req)
{ 
    sequence_status=1;
    sequence_no=sequence_no+1;
    if(sequence_no>3){
        sequence_no=1;
    }
    printf("status sequence:%d\n",sequence_no);
    return ESP_OK;
}


/************************************************************************************************************
*
*       Konfig timerów ird itp
*
*************************************************************************************************************/
static void init_hw(void){
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
        };

        ledc_timer_config(&ledc_timer);

    for(int i=0;i<COLOR_AMOUNT;i++){
        ledc_channel[i].channel = LEDC_CHANNEL_0+i;
        ledc_channel[i].duty = 0;
        ledc_channel[i].gpio_num = COLOR_PINS[i];
        ledc_channel[i].speed_mode = LEDC_HIGH_SPEED_MODE;
        ledc_channel[i].hpoint = 0;
        ledc_channel[i].timer_sel = LEDC_TIMER_0;
        ledc_channel_config(&ledc_channel[i]);
    }
}
    

void color_regulator(void *arg)   /// OGARNIJ TO GÓWNO!!!!
{
    while(1){
        //  Obsluga pojedynczych kolorów tj. Red, Green, Blue, White, Black
        if (sequence_status == false) {
            int RGBvalue[3]={r_value,g_value,b_value};
            for(int j=0;j<COLOR_AMOUNT;j++){
                ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
            }
            sys_delay_ms(50);
        }



        // Sekwencje
        else if (sequence_status==true) {
            switch (sequence_no){
                //Sekwencja no. 1 - R -> G -> B -> R
                case 1 :
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=1){
                        break;
                    }
                    r_value=255-i,g_value=i,b_value=0;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                    }
                    sys_delay_ms(50);
                }
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=1){
                        break;
                    }
                    r_value=0,g_value=255-i,b_value=i;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                        
                    }
                    sys_delay_ms(50);
                    }
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=1){
                        break;
                    }
                    r_value=i,g_value=0,b_value=255-i;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                        
                    }
                    sys_delay_ms(50);
                    }
                    break;



                case 2 :
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=2){
                        break;
                    }
                    r_value=255-i,g_value=i,b_value=255-i;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                    }
                    sys_delay_ms(50);
                }
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=2){
                        break;
                    }
                    r_value=i,g_value=255,b_value=0;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                        
                    }
                    sys_delay_ms(50);
                    }
                    for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=2){
                        break;
                    }
                    r_value=255,g_value=255-i,b_value=i;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                        
                    }
                    sys_delay_ms(50);
                    }
                    break;

                case 3 :
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=3){
                        break;
                    }
                    r_value=200,g_value=i,b_value=0;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                    }
                    sys_delay_ms(50);
                }
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=3){
                        break;
                    }
                    r_value=200,g_value=255-i,b_value=0;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                        
                    }
                    sys_delay_ms(50);
                    }
                for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=3){
                        break;
                    }
                    r_value=200,g_value=0,b_value=i;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                        
                    }
                    sys_delay_ms(50);
                    }
                    for (int i=0;i<255;i++){
                    if (sequence_status==false || sequence_no!=3){
                        break;
                    }
                    r_value=200,g_value=0,b_value=255-i;
                    int RGBvalue[3]={r_value,g_value,b_value};
                    for(int j=0;j<COLOR_AMOUNT;j++){
                        ledc_set_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel, RGBvalue[j]);
                        ledc_update_duty(ledc_channel[j].speed_mode, ledc_channel[j].channel);
                        
                    }
                    sys_delay_ms(50);
                    }
                    break;







            }
        }


        sys_delay_ms(50);
    }
}


/************************************************************************************************************
*
*       główny
*
*************************************************************************************************************/
void app_main()
{
    //fs_main();
        // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    if (ESP_OK == fs_mount()) {
        xTaskCreatePinnedToCore(http_server_main,"http_server",16384,NULL,10,&myTaskHandleHTTP,1);
        ESP_LOGI(TAG, "mounted");
    } else {
        ESP_LOGI(TAG, "mounting fail");
    }
    
    // http_server_main();
    // http_server_InitWifi();
    ESP_LOGI(TAG, "LED Control Web Server is running ... ...\n");
    init_hw();
    
    // setup_server(); 
    xTaskCreatePinnedToCore(color_regulator,"color_regulator",16384,NULL,10,&myTaskHandle2,1);
    vTaskDelay(300);

    /*TODO:
    default gway to web site
    ram usage
    free memory
    flash usage
    files list on flash
    date and time? + actual time of working? maybe...*/
}

