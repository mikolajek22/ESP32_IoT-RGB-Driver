#include "http_server.h"
#include "fs.h"

/* Flags */
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1

/* User configuration */
#define CONFIG_ESP_WIFI_SSID        "UPC6591066"        
#define CONFIG_ESP_WIFI_PASSWORD    "uEuyknbts4rt"      
#define CONFIG_ESP_MAXIMUM_RETRY    5                   

#define TAG_WIFI_MODULE             0
#define TAG_HTTP_SERVER             1
static const char *TAG[2] = {"WIFI_MODULE", "HTTP_SERVER"};

#define MAX_PAGE_SIZE               8192
#define READ_SIZE                   255

static EventGroupHandle_t wifiEventGroup;
static int retNum = 0;

static void Wifi_EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t Wifi_SetupConnection(void);

httpd_handle_t setup_server(void);

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

/* WIFI SETUP */
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
        retNum = 0;
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
}


esp_err_t Wifi_SetupConnection(void) {

    // TODO: change esp to be both station and router : 192.168.0.10 !
    wifiEventGroup = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

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


/* SERVER SETUP */
esp_err_t  get_EventHandler(httpd_req_t *req) {
    char*       buffer;
    char*       pageText;
    uint8_t     fileID;
    size_t      readBytes;
    uint16_t    totalReadBytes = 0;
    size_t bufferLen = httpd_req_get_url_query_len(req);
    
    if (bufferLen > 0) {
        buffer = malloc(bufferLen+1);
        memset(buffer, 0, bufferLen+1);
        if (httpd_req_get_url_query_str(req, buffer, bufferLen+1) == ESP_OK){
            char key[64];
            //  example: 192.168.0.10/get?page=main
            /* TODO: tak powinna być zrobiona strona. Tj klikasz przycisk, wczytywana jest nowa strone z innym key value.*/
            if (httpd_query_key_value(buffer,"page", key, 64) == ESP_OK) {
                if (!strcmp(key, "main")) {
                    ESP_LOGW(TAG[1],"SENDING MAIN");
                    ESP_LOGI(TAG[1], "FINDING ID");
                    if (fs_findID(&fileID) == ESP_OK) { 
                        ESP_LOGI(TAG[1], "MAALLOCKING");
                        pageText = malloc(MAX_PAGE_SIZE);
                        if (pageText == NULL){
                            ESP_LOGE(TAG[1],"KURWA MALLOC FAIL");
                        }
                        ESP_LOGI(TAG[1], "MALLOCK DONE");
                        if (ESP_OK == fs_openFile(fileID, "main_page.html")){
                            ESP_LOGI(TAG[1], "file open");
                        }
                        else{
                            ESP_LOGW(TAG[1], "file open fail");
                        }
                        do {
                            readBytes = fs_readFile(fileID, "main_page.html", pageText+totalReadBytes,totalReadBytes);
                            ESP_LOGI(TAG[1], "READED %d", totalReadBytes);
                            totalReadBytes += readBytes;
                        } while(readBytes == READ_SIZE);
                        if (totalReadBytes > 0) {
                            ESP_LOGI(TAG[1],"PLIK HTML:: %s", pageText);
                            ESP_LOGI(TAG[1],"PLIK HTML:: %d", sizeof(pageText));
                            if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                ESP_LOGI(TAG[1], "Main page has been sent");
                            }
                            else {
                                ESP_LOGE(TAG[1], "Main page has not been sent. Read bytes from file is equal to 0!");
                            }
                        }
                        free(pageText);
                    }
                }
                else if (!strcmp(key, "config")) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_PAGE_SIZE);
                        do {
                            readBytes = fs_readFile(fileID, "config_page.html", pageText+totalReadBytes,totalReadBytes);
                            totalReadBytes += readBytes;
                        } while(readBytes == READ_SIZE);
                        if (totalReadBytes > 0) {
                            if (httpd_resp_send(req, pageText, sizeof(pageText)) == ESP_OK) {
                                ESP_LOGI(TAG[1], "Config page has been sent");
                            }
                            else {
                                ESP_LOGE(TAG[1], "Config page has not been sent. Read bytes from file is equal to 0!");
                            }
                        }
                        free(pageText);
                    }
                }
            }
        }
        else {
            ESP_LOGW(TAG[1],"buffer, %s",buffer);
            ESP_LOGW(TAG[1],"SENDING DEFAULT");
            if (fs_findID(&fileID) == ESP_OK) { 
                        ESP_LOGI(TAG[1], "MAALLOCKING");
                        pageText = malloc(MAX_PAGE_SIZE);
                        ESP_LOGI(TAG[1], "MALLOCK DONE");
                        if (ESP_OK == fs_openFile(fileID, "main_page.html")){
                            ESP_LOGI(TAG[1], "file open");
                        }
                        else{
                            ESP_LOGW(TAG[1], "file open fail");
                        }
                        do {
                            readBytes = fs_readFile(fileID, "main_page.html", pageText+totalReadBytes,totalReadBytes);
                            totalReadBytes += readBytes;
                            ESP_LOGI(TAG[1], "READED %d", totalReadBytes);
                        } while(readBytes == READ_SIZE);
                        ESP_LOGI(TAG[1],"PLIK HTML:: %s", pageText);
                        ESP_LOGI(TAG[1],"PLIK HTML:: %d", sizeof(pageText));
                        if (totalReadBytes > 0) {
                            if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                ESP_LOGI(TAG[1], "Main page has been sent");
                            }
                            else {
                                ESP_LOGE(TAG[1], "Main page has not been sent. Read bytes from file is equal to 0!");
                            }
                        }
                        free(pageText);
                    }
            else {
                ESP_LOGE(TAG[1], "No free file enabled.");
            }
        }
    }
    return ESP_OK;
}

esp_err_t  post_EventHandler(httpd_req_t *req) {
    return 1;
}

httpd_uri_t uri_get = {
    .uri = "/get",
    .method = HTTP_GET,
    .handler = get_EventHandler,
    .user_ctx = NULL };

httpd_uri_t uri_post = {
    .uri = "/post",
    .method = HTTP_GET,
    .handler = post_EventHandler,
    .user_ctx = NULL };


httpd_handle_t setup_server(void)
{
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }

    return server;
}