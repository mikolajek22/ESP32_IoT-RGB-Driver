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
#define WIFI_MODE                   "WIFI_STA_DEF"
static const char *TAG[2] = {"WIFI_MODULE", "HTTP_SERVER"};

static EventGroupHandle_t wifiEventGroup;

static void Wifi_EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t Wifi_SetupConnection(void);

httpd_handle_t setup_server(void);
extern esp_netif_ip_info_t connectionInfo;
static bool serverSet = false;

esp_err_t http_server_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Write readed configuration from settings.json.
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey(WIFI_MODE);
    IP4_ADDR(&ip_info.ip, defaultCfg.ipAddr[0], defaultCfg.ipAddr[1], defaultCfg.ipAddr[2], defaultCfg.ipAddr[3]);
    IP4_ADDR(&ip_info.netmask, defaultCfg.netmask[0], defaultCfg.netmask[1], defaultCfg.netmask[2], defaultCfg.netmask[3]);
    IP4_ADDR(&ip_info.gw, defaultCfg.defaultGw[0], defaultCfg.defaultGw[1], defaultCfg.defaultGw[2], defaultCfg.defaultGw[3]);
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
    return ESP_OK;
}

esp_err_t http_server_connect() {
    /* wifi connect */
    if (ESP_OK == Wifi_SetupConnection()) {
        ip_addr_t dns_primary;
        IP_ADDR4(&dns_primary, 8, 8, 8, 8); // Google DNS
        dns_setserver(0, &dns_primary);
        return ESP_OK;
    }
    else {
        return ESP_FAIL;
    }  
}

/* WiFi status handler */
static void Wifi_EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static uint16_t nexTimeWait;
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            nexTimeWait = 0;
            esp_wifi_connect(); 
        } 
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if(nexTimeWait != 0) {   
                ESP_LOGW(TAG[TAG_WIFI_MODULE], "Wi-Fi disconnected. Reconnecting in %d seconds", nexTimeWait / 1000);
                vTaskDelay(nexTimeWait / portTICK_PERIOD_MS);
            }
            /* retry to connect after set time (eacnh retry increment delay by 5 seconds (untill 30 seconds))*/
            if (nexTimeWait < TIME_WAIT_TO_CONN_MAX) {
                nexTimeWait += TIME_WAIT_TO_CONN_BASE;
            }
            esp_wifi_connect();
        }
        else if (event_id == WIFI_EVENT_STA_BEACON_TIMEOUT) {
            wsInfo.isActive = false; // if conn is lost - disable WS socket (logging) send.
            vTaskDelay(100 / portTICK_PERIOD_MS);
            ESP_LOGE(TAG[TAG_WIFI_MODULE], "Wi-Fi connection error");
        }
        
        else {
            ESP_LOGI(TAG[TAG_WIFI_MODULE], "WiFi event: %ld", event_id);
        }
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG[TAG_WIFI_MODULE], "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        connectionInfo = event->ip_info;

        if (!serverSet) {
            if (setup_server() != NULL) {
                serverSet = true;
                ESP_LOGI(TAG[TAG_HTTP_SERVER], "Succeed to start HTTP server");
            } else {
                ESP_LOGE(TAG[TAG_HTTP_SERVER], "Failed to start HTTP server.");
            }
        }
        nexTimeWait = 0;
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT){
       printf("IP event: %ld", event_id);
    }
}

/* Setting up WiFi connection*/
esp_err_t Wifi_SetupConnection(void) {
    // TODO: change esp to be both station and router : 192.168.0.10 ! STA OR AP
    wifiEventGroup = xEventGroupCreate();

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

    /* Wait until flag changed - fail or connected */
    EventBits_t bits = xEventGroupWaitBits(wifiEventGroup,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG[TAG_WIFI_MODULE], "Connected to ap SSID:%s password:%s",
                 defaultCfg.wifiName, defaultCfg.wifiPassword);
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG[TAG_WIFI_MODULE], "Failed to connect to SSID:%s, password:%s",
                 defaultCfg.wifiName, defaultCfg.wifiPassword);
    }
    else {
        ESP_LOGW(TAG[TAG_WIFI_MODULE], "UNEXPECTED EVENT");
    }
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
    httpd_handle_t server = NULL;
    config.max_uri_handlers = 16;

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