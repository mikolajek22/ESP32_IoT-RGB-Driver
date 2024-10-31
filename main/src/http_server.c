#include "http_server.h"
#include "fs.h"
#include "esp_idf_version.h"
#include "cJSON.h"

/* Flags */
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1

/* User configuration */
#define STATIC_IP_ENABLED           true
static const uint8_t staticIP[4]    ={192, 168, 0, 220};
#define CONFIG_ESP_WIFI_SSID        "UPC6591066"        
#define CONFIG_ESP_WIFI_PASSWORD    "uEuyknbts4rt"      
#define CONFIG_ESP_MAXIMUM_RETRY    5                   

#define TAG_WIFI_MODULE             0
#define TAG_HTTP_SERVER             1
static const char *TAG[2] = {"WIFI_MODULE", "HTTP_SERVER"};

#define MAX_PAGE_SIZE               16384
#define READ_SIZE                   255

#define HTTP_PAGE_PARAMETER_NAME            "page"

#define HTTP_PAGE_PARAMETER_MAIN_NAME       "main"
#define HTTP_PAGE_NAME_MAIN                 "main_page.html"
#define HTTP_PAGE_PARAMETER_CONFIG_NAME     "config"
#define HTTP_PAGE_NAME_CONFIG               "config_page.html"
#define HTTP_PAGE_PARAMETER_CONTROL_NAME    "control"
#define HTTP_PAGE_NAME_CONTROL              "control_page.html"

static EventGroupHandle_t wifiEventGroup;
static int retNum = 0;

static void Wifi_EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t Wifi_SetupConnection(void);

httpd_handle_t setup_server(void);
static esp_netif_ip_info_t connectionInfo;

extern void actualizeValue(uint8_t red, uint8_t green, uint8_t blue);

void http_server_main(void){
    const uint16_t maxTimeWait  = 30000;
    const uint16_t baseTimeWait = 5000;
    uint16_t nexTimeWait        = baseTimeWait;
    wifi_ap_record_t wifiRecord;
    bool connectionSet = false;
    bool serverSet = false;
    while (true) {
        
        /* set up wifi connection */
        if (!connectionSet){
            while (ESP_OK != Wifi_SetupConnection()) {
                ESP_LOGE(TAG[TAG_WIFI_MODULE], "Unable to setup connection, next retry in: %d seconds\n", nexTimeWait);
                vTaskDelay(nexTimeWait);
                if (nexTimeWait < maxTimeWait) {
                    nexTimeWait =+ nexTimeWait;
                }
            }
            nexTimeWait = baseTimeWait;
            connectionSet = true;
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

    // TODO: change esp to be both station and router : 192.168.0.10 !
    wifiEventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
#ifdef STATIC_IP_ENABLED
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, staticIP[0], staticIP[1], staticIP[2], staticIP[3]);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_dhcpc_stop(netif); // Wyłącz DHCP Client
    esp_netif_set_ip_info(netif, &ip_info);
#endif
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
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
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
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG[TAG_WIFI_MODULE], "Failed to connect to SSID:%s, password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    }
    else {
        ESP_LOGE(TAG[TAG_WIFI_MODULE], "UNEXPECTED EVENT");
    }
    vEventGroupDelete(wifiEventGroup);
    return (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
}

/* Get .html default page */
esp_err_t getStart_EventHandler(httpd_req_t *req) {
    char*       pageText;
    uint8_t     fileID;
    size_t      readBytes;
    uint16_t    totalReadBytes = 0;
    if (fs_findID(&fileID) == ESP_OK) { 
        pageText = malloc(MAX_PAGE_SIZE);
        if (pageText == NULL) {
            ESP_LOGE(TAG[1],"Not enough memory, malloc failed!");
        }
        else {
            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_MAIN)) {
                do {
                    readBytes = fs_readFile(fileID, HTTP_PAGE_NAME_MAIN, pageText + totalReadBytes, totalReadBytes);
                    totalReadBytes += readBytes;
                } while (readBytes == READ_SIZE);
                if (totalReadBytes > 0) {
                    ESP_LOGI(TAG[1], "File to send: \nFile name: %s \nFile size: %d bytes", HTTP_PAGE_NAME_MAIN, totalReadBytes);
                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                        ESP_LOGI(TAG[1], "%s page has been sent", HTTP_PAGE_NAME_MAIN);
                    }
                    else {
                        ESP_LOGE(TAG[1], "%s page sending error!", HTTP_PAGE_NAME_MAIN);
                    }
                }
                else {
                    ESP_LOGE(TAG[1], "readed bytes from file: %s is equal to: %d!", HTTP_PAGE_NAME_MAIN, totalReadBytes);
                }
                fs_closeFile(fileID);
            }
            else {
                ESP_LOGE(TAG[1], "File opening error: %s", HTTP_PAGE_NAME_MAIN);
            }
        }
        free(pageText);
    }
    else {
        ESP_LOGE(TAG[1], "All file handlers are busy!");
    } 
    return ESP_OK;
}

/* Get .html pages */
esp_err_t  getPage_EventHandler(httpd_req_t *req) {
    char*       buffer;
    char*       pageText;
    uint8_t     fileID;
    size_t      readBytes;
    uint16_t    totalReadBytes = 0;
    size_t      bufferLen = httpd_req_get_url_query_len(req);
    
    if (bufferLen > 0) {
        buffer = malloc(bufferLen+1);
        memset(buffer, 0, bufferLen+1);
        if (httpd_req_get_url_query_str(req, buffer, bufferLen+1) == ESP_OK) {
            char key[64];
            //  example: 192.168.0.10/get?page=main
            if (httpd_query_key_value(buffer,HTTP_PAGE_PARAMETER_NAME, key, 64) == ESP_OK) {
            
                /* SENDING MAIN PAGE */
                if (!strcmp(key, HTTP_PAGE_PARAMETER_MAIN_NAME)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_PAGE_SIZE);
                        if (pageText == NULL) {
                            ESP_LOGE(TAG[1],"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_MAIN)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTTP_PAGE_NAME_MAIN, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG[1], "File to send: \nFile name: %s \nFile size: %d bytes", HTTP_PAGE_NAME_MAIN, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG[1], "%s page has been sent", HTTP_PAGE_NAME_MAIN);
                                    }
                                    else {
                                        ESP_LOGE(TAG[1], "%s page sending error!", HTTP_PAGE_NAME_MAIN);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG[1], "readed bytes from file: %s is equal to: %d!", HTTP_PAGE_NAME_MAIN, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG[1], "File opening error: %s", HTTP_PAGE_NAME_MAIN);
                            }
                        }
                        free(pageText);
                    }
                    else {
                        ESP_LOGE(TAG[1], "All file handlers are busy!");
                    }
                }

                /* SENDING CONFIG PAGE */
                else if (!strcmp(key, HTTP_PAGE_PARAMETER_CONFIG_NAME)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_PAGE_SIZE);
                        if (pageText == NULL){
                            ESP_LOGE(TAG[1],"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_CONFIG)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTTP_PAGE_NAME_CONFIG, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG[1], "File to send: \nFile name: %s \nFile size: %d bytes", HTTP_PAGE_NAME_CONFIG, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG[1], "%s page has been sent", HTTP_PAGE_NAME_CONFIG);
                                    }
                                    else {
                                        ESP_LOGE(TAG[1], "%s page sending error!", HTTP_PAGE_NAME_CONFIG);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG[1], "readed bytes from file: %s is equal to: %d!", HTTP_PAGE_NAME_CONFIG, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG[1], "File opening error: %s", HTTP_PAGE_NAME_CONFIG);
                            }
                        }
                        free(pageText);
                    }
                    else {
                        ESP_LOGE(TAG[1], "All file handlers are busy!");
                    }
                }

                /* SENDING CONTROL PAGE */
                else if (!strcmp(key, HTTP_PAGE_PARAMETER_CONTROL_NAME)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_PAGE_SIZE);
                        if (pageText == NULL){
                            ESP_LOGE(TAG[1],"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_CONTROL)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTTP_PAGE_NAME_CONTROL, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG[1], "File to send: \nFile name: %s \nFile size: %d bytes", HTTP_PAGE_NAME_CONTROL, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG[1], "%s page has been sent", HTTP_PAGE_NAME_CONTROL);
                                    }
                                    else {
                                        ESP_LOGE(TAG[1], "%s page sending error!", HTTP_PAGE_NAME_CONTROL);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG[1], "readed bytes from file: %s is equal to: %d!", HTTP_PAGE_NAME_CONTROL, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG[1], "File opening error: %s", HTTP_PAGE_NAME_CONTROL);
                            }
                        }
                        free(pageText);
                    }
                    else {
                        ESP_LOGE(TAG[1], "All file handlers are busy!");
                    }
                }

                else {
                    ESP_LOGE(TAG[1], "Invalid value of querry parameter.");
                }
            }
            else {
                ESP_LOGE(TAG[1], "Invalid name of querry parameter.");
            }
        }
        else {
            ESP_LOGE(TAG[1], "Invalid querry string.");
        }
    }
    return ESP_OK;
}

/* Get information about ESP32 */
esp_err_t getInfo_EventHandler(httpd_req_t *req) {
    const char* json_response[256]; 
    sprintf(json_response,
        "{\"wifiName\":\"%s\",\"wifiPass\":\"%s\",\"ipAddr\":\""IPSTR"\",\"subnetMask\":\""IPSTR"\",\"fwV\":\"%d.%d.%d\"}", 
        CONFIG_ESP_WIFI_SSID,
        CONFIG_ESP_WIFI_PASSWORD,
        IP2STR(&connectionInfo.ip),
        IP2STR(&connectionInfo.netmask),
        ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));
    return ESP_OK;
}

/* TODO: handler of RGB control */
esp_err_t  postRGB_EventHandler(httpd_req_t *req) {
    const char* postBuffer[256];
    size_t      contentLen = req->content_len;
    uint8_t     red = 0;
    uint8_t     green = 0;
    uint8_t     blue = 0;

    if (httpd_req_recv(req, postBuffer, contentLen) > 0) {
        cJSON *root = cJSON_Parse(postBuffer);
        cJSON *rgbValues = cJSON_GetObjectItem(root, "rgbValues");
        red = cJSON_GetObjectItem(rgbValues, "red")->valueint;
        green = cJSON_GetObjectItem(rgbValues, "green")->valueint;
        blue = cJSON_GetObjectItem(rgbValues, "blue")->valueint;
        actualizeValue(red, green, blue);
    }
    
    return ESP_OK;
}

/* Get config */
httpd_uri_t get_page = {
    .uri = "/get",
    .method = HTTP_GET,
    .handler = getPage_EventHandler,
    .user_ctx = NULL };

httpd_uri_t get_default = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = getStart_EventHandler,
    .user_ctx = NULL };

httpd_uri_t get_info = {
    .uri = "/info",
    .method = HTTP_GET,
    .handler = getInfo_EventHandler,
    .user_ctx = NULL };

/* Post config */
httpd_uri_t uri_post = {
    .uri = "/RGB",
    .method = HTTP_PUT,
    .handler = postRGB_EventHandler,
    .user_ctx = NULL };

/* Set up http server */
httpd_handle_t setup_server(void)
{   
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &get_default);
        httpd_register_uri_handler(server, &get_page);
        httpd_register_uri_handler(server, &get_info);
        httpd_register_uri_handler(server, &uri_post);
    }
    return server;
}