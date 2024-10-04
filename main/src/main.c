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
// #include "driver/adc.h"

#define RED_LED_PIN 13
#define GREEN_LED_PIN 12
#define BLUE_LED_PIN 14

#define SAMPLE_CNT 32

#define  CONFIG_ESP_WIFI_SSID "UPC6591066"
#define CONFIG_ESP_WIFI_PASSWORD "uEuyknbts4rt"
#define CONFIG_ESP_MAXIMUM_RETRY 5

static ledc_channel_config_t ledc_channel[3];
int COLOR_AMOUNT = 3;
int COLOR_PINS[3]={RED_LED_PIN,GREEN_LED_PIN, BLUE_LED_PIN};
bool sequence_status;
int sequence_no;
bool breakFlag;
char on_resp[];


static const char *TAG = "espressif"; // TAG for debug

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;

int r_value=0, g_value=0, b_value=0;
int *pr_value=&r_value, *pg_value=&g_value, *pb_value=&b_value; //pointery do wartości

TaskHandle_t myTaskHandle = NULL;
TaskHandle_t myTaskHandle2 = NULL;
/************************************************************************************************************
*
*       Łączenie płytki z wifi - obsługa błędów albo poprawnego połączenia.
*
*************************************************************************************************************/
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


/************************************************************************************************************
*       
*           inicjalizacja połączenia przez wifi.
*
*************************************************************************************************************/
void connect_wifi(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    vEventGroupDelete(s_wifi_event_group);
}

esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    response = httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);
    return response;
}
esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}
esp_err_t change_color_handler_red(httpd_req_t *req)
{
    r_value=255;
    g_value=0;
    b_value=0;
    sequence_status=false;
    sequence_no=0;
    return ESP_OK;
}
esp_err_t change_color_handler_green(httpd_req_t *req)
{
    r_value=0;
    g_value=255;
    b_value=0;
    sequence_status=false;
    sequence_no=0;
    return ESP_OK;
}
esp_err_t change_color_handler_blue(httpd_req_t *req)
{
    r_value=0;
    g_value=0;
    b_value=255;
    sequence_status=false;
    sequence_no=0;
    return ESP_OK;
}
esp_err_t change_color_handler_white(httpd_req_t *req)
{
    r_value=255;
    g_value=255;
    b_value=255;
    sequence_status=false;
    sequence_no=0;
    return ESP_OK;
}
esp_err_t change_color_handler_off(httpd_req_t *req)
{
    r_value=0;
    g_value=0;
    b_value=0;
    sequence_status=false;
    sequence_no=0;
    return ESP_OK;
}
esp_err_t change_color_handler(httpd_req_t *req)
{
    
    char r_str[4];
    char g_str[4];
    char b_str[4];
    sequence_status=false;
    sequence_no=0;
    printf("URI: %s\n", req->uri);
    // Pobierz kolory RGB z parametrów GET
    if (httpd_query_key_value(req->uri, "r", r_str, sizeof(r_str)) == ESP_OK) {
        *pr_value = atoi(r_str);
    }
    if (httpd_query_key_value(req->uri, "g", g_str, sizeof(g_str)) == ESP_OK) {
        *pg_value = atoi(g_str);
    }
    if (httpd_query_key_value(req->uri, "b", b_str, sizeof(b_str)) == ESP_OK) {
        *pb_value = atoi(b_str);
    }
    char response_buffer[64];
    snprintf(response_buffer, sizeof(response_buffer), "Nowy kolor: R=%d, G=%d, B=%d", r_value, g_value, b_value);
    httpd_resp_send(req, response_buffer, strlen(response_buffer));
    return ESP_OK;
}


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


//      URI GET

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL};

//      COLOR CHANGE

httpd_uri_t change_color_uri = {
    .uri = "/zmien_kol",
    .method = HTTP_GET,
    .handler = change_color_handler,
    .user_ctx = NULL
};
httpd_uri_t change_color_uri_red = {
    .uri = "/RED",
    .method = HTTP_GET,
    .handler = change_color_handler_red,
    .user_ctx = NULL
};
httpd_uri_t change_color_uri_green = {
    .uri = "/GREEN",
    .method = HTTP_GET,
    .handler = change_color_handler_green,
    .user_ctx = NULL
};
httpd_uri_t change_color_uri_blue = {
    .uri = "/BLUE",
    .method = HTTP_GET,
    .handler = change_color_handler_blue,
    .user_ctx = NULL
};
httpd_uri_t change_color_uri_white = {
    .uri = "/WHITE",
    .method = HTTP_GET,
    .handler = change_color_handler_white,
    .user_ctx = NULL
};
httpd_uri_t change_color_uri_off = {
    .uri = "/OFF",
    .method = HTTP_GET,
    .handler = change_color_handler_off,
    .user_ctx = NULL

    //      SEQUENCES

};
httpd_uri_t change_color_uri_sequence = {
    .uri = "/SEQ_1",
    .method = HTTP_GET,
    .handler = change_color_sequence,
    .user_ctx = NULL
};


httpd_handle_t setup_server(void)
{
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &change_color_uri);
        httpd_register_uri_handler(server, &change_color_uri_red);
        httpd_register_uri_handler(server, &change_color_uri_green);
        httpd_register_uri_handler(server, &change_color_uri_blue);
        httpd_register_uri_handler(server, &change_color_uri_white);
        httpd_register_uri_handler(server, &change_color_uri_off);
        httpd_register_uri_handler(server, &change_color_uri_sequence);


    }

    return server;
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
    connect_wifi();
    ESP_LOGI(TAG, "LED Control Web Server is running ... ...\n");
    init_hw();
    
    setup_server(); 
    xTaskCreatePinnedToCore(color_regulator,"color_regulator",16384,NULL,10,&myTaskHandle2,1);
    vTaskDelay(300);
}

