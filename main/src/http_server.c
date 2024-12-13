#include "http_server.h"
#include "fs.h"
#include "esp_idf_version.h"
#include "cJSON.h"
#include "startup.h"
#include "http_handlers.h"
#include "lwip/dns.h"
#include "sntp_sync.h"

/* Flags */
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1

/* User configuration */
#define CONFIG_ESP_MAXIMUM_RETRY    5       
#define TIME_WAIT_TO_CONN_MAX       30000
#define TIME_WAIT_TO_CONN_BASE      5000          

#define TAG_WIFI_MODULE             0
#define TAG_HTTP_SERVER             1
static const char *TAG[2] = {"WIFI_MODULE", "HTTP_SERVER"};


static EventGroupHandle_t wifiEventGroup;
static int retNum = 0;

static void Wifi_EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t Wifi_SetupConnection(void);

httpd_handle_t setup_server(void);
extern esp_netif_ip_info_t connectionInfo;


void http_server_main(void){
    uint16_t nexTimeWait = TIME_WAIT_TO_CONN_BASE;
    wifi_ap_record_t wifiRecord;
    bool connectionSet = false;
    bool serverSet = false;
    while (true) {

        /* set up wifi connection */
        if (!connectionSet){
            while (ESP_OK != Wifi_SetupConnection()) {
                ESP_LOGE(TAG[TAG_WIFI_MODULE], "Unable to setup connection, next retry in: %d seconds\n", nexTimeWait);
                vTaskDelay(nexTimeWait);
                if (nexTimeWait < TIME_WAIT_TO_CONN_MAX) {
                    nexTimeWait =+ nexTimeWait;
                }
            }
            nexTimeWait = TIME_WAIT_TO_CONN_BASE;
            connectionSet = true;
            ip_addr_t dns_primary;
            IP_ADDR4(&dns_primary, 8, 8, 8, 8); // Google DNS
            dns_setserver(0, &dns_primary);
            sntp_sync_init();
        }
        uint8_t serverRet = 0;

        /* set up http server */
        if (!serverSet){
            while (serverRet < 3) {
                if (setup_server() != NULL) {
                    ESP_LOGI(TAG[1], "Http Server is active.");
                    serverSet = true;
                    break;
                }
                else {
                    serverRet++;
                    ESP_LOGE(TAG[1], "Failed to set up HTTP Server, ret no. %d", serverRet);
                    vTaskDelay(1000);
                }
            }
        }
    
        /* check if connection still established*/
        if (serverSet & connectionSet) {
            if (esp_wifi_sta_get_ap_info(&wifiRecord) != ESP_OK) {
                serverSet = false;
                connectionSet = false;
            }
        }
    }
}

/* WiFi status handler */
static void Wifi_EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retNum < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            retNum++;
            ESP_LOGI(TAG[TAG_WIFI_MODULE], "retry to connect to the AP");
        }
        else {
            xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG[TAG_WIFI_MODULE], "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG[TAG_WIFI_MODULE], "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        connectionInfo = event->ip_info;
        retNum = 0;
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
}

/* Setting up WiFi connection*/
esp_err_t Wifi_SetupConnection(void) {

    // TODO: change esp to be both station and router : 192.168.0.10 ! STA OR AP
    wifiEventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, defaultCfg.ipAddr[0], defaultCfg.ipAddr[1], defaultCfg.ipAddr[2], defaultCfg.ipAddr[3]);
    IP4_ADDR(&ip_info.netmask, defaultCfg.netmask[0], defaultCfg.netmask[1], defaultCfg.netmask[2], defaultCfg.netmask[3]);
    IP4_ADDR(&ip_info.gw, defaultCfg.defaultGw[0], defaultCfg.defaultGw[1], defaultCfg.defaultGw[2], defaultCfg.defaultGw[3]);
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_dhcpc_stop(netif);
    esp_netif_set_ip_info(netif, &ip_info);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instanceAnyId;
    esp_event_handler_instance_t instanceGotIp;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &Wifi_EventHandler,
                                                        NULL,
                                                        &instanceAnyId));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &Wifi_EventHandler,
                                                        NULL,
                                                        &instanceGotIp));
 
    wifi_config_t wifiConfig = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    strncpy((char*)wifiConfig.sta.ssid, defaultCfg.wifiName, sizeof(wifiConfig.sta.ssid) - 1);
    strncpy((char*)wifiConfig.sta.password, defaultCfg.wifiPassword, sizeof(wifiConfig.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG[TAG_WIFI_MODULE], "wifi_init_sta finished.");

    /* Wait until flag changed - fail or connected */
    EventBits_t bits = xEventGroupWaitBits(wifiEventGroup,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG[TAG_WIFI_MODULE], "connected to ap SSID:%s password:%s",
                 defaultCfg.wifiName, defaultCfg.wifiPassword);
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG[TAG_WIFI_MODULE], "Failed to connect to SSID:%s, password:%s",
                 defaultCfg.wifiName, defaultCfg.wifiPassword);
    }
    else {
        ESP_LOGE(TAG[TAG_WIFI_MODULE], "UNEXPECTED EVENT");
    }
    vEventGroupDelete(wifiEventGroup);
    return (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
}


/* Get config */
httpd_uri_t get_page = {
    .uri = "/get",
    .method = HTTP_GET,
    .handler = http_handlers_getPage_EventHandler,
    .user_ctx = NULL };

httpd_uri_t get_default = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = http_handlers_getStartPage_EventHandler,
    .user_ctx = NULL };

httpd_uri_t get_info = {
    .uri = "/info",
    .method = HTTP_GET,
    .handler = http_handlers_getInfo_EventHandler,
    .user_ctx = NULL };

httpd_uri_t get_download = {
    .uri = "/download",
    .method = HTTP_GET,
    .handler = http_handlers_getDownload_EventHandler,
    .user_ctx = NULL };

/* Handlers configuration */
httpd_uri_t uri_put_rgb = {
    .uri = "/RGB",
    .method = HTTP_PUT,
    .handler = http_handlers_postRGB_EventHandler,
    .user_ctx = NULL };

httpd_uri_t uri_post_configuration = {
    .uri = "/configuration",
    .method = HTTP_POST,
    .handler = http_handlers_postConfiguration_EventHandler,
    .user_ctx = NULL };

httpd_uri_t uri_get_logs = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = http_handlers_websocketEnable_EventHandler,
    .user_ctx = NULL,
    .is_websocket = true };

httpd_handle_t setup_server(void)
{   
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &get_default);
        httpd_register_uri_handler(server, &get_page);
        httpd_register_uri_handler(server, &get_info);
        httpd_register_uri_handler(server, &uri_post_configuration);
        httpd_register_uri_handler(server, &uri_put_rgb);
        httpd_register_uri_handler(server, &get_download);
        httpd_register_uri_handler(server, &uri_get_logs);
    }
    return server;
}