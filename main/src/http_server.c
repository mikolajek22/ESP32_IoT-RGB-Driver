#include "http_server.h"
#include "fs.h"
#include "esp_idf_version.h"
#include "cJSON.h"

/* Flags */
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1

/* User configuration */
#define STATIC_IP_ENABLED           true

// #define CONFIG_ESP_WIFI_SSID        "UPC6591066"        
// #define CONFIG_ESP_WIFI_PASSWORD    "uEuyknbts4rt"    
#define CONFIG_ESP_WIFI_SSID        "Zyxel_96C1"        
#define CONFIG_ESP_WIFI_PASSWORD    "M7KNTYLG7K"  
#define CONFIG_ESP_MAXIMUM_RETRY    5                   

#define TAG_WIFI_MODULE             0
#define TAG_HTTP_SERVER             1
static const char *TAG[2] = {"WIFI_MODULE", "HTTP_SERVER"};

#define MAX_PAGE_SIZE               24576
#define MAX_CFG_FILE_SIZE           4096
#define READ_SIZE                   255

#define HTTP_PAGE_PARAMETER_NAME            "page"

#define HTTP_PAGE_PARAMETER_MAIN_NAME       "main"
#define HTTP_PAGE_NAME_MAIN                 "main_page.html"
#define HTTP_PAGE_PARAMETER_CONFIG_NAME     "config"
#define HTTP_PAGE_NAME_CONFIG               "config_page.html"
#define HTTP_PAGE_PARAMETER_CONTROL_NAME    "control"
#define HTTP_PAGE_NAME_CONTROL              "control_page.html"
#define HTTP_PAGE_PARAMETER_LOGS_NAME       "logs"
#define HTTP_PAGE_NAME_LOGS                 "logs_page.html"

#define SETTING_FILE_NAME                   "settings.json"
static const uint8_t defaultStaticIP[4]              ={192, 168, 0, 10};
static const uint8_t defaultStaticMask[4]            ={255, 255, 255, 0};
static const uint8_t defaultStaticGateway[4]         ={192, 168, 0, 10};

static uint8_t staticIP[4]              ={192, 168, 0, 10};
static uint8_t staticMask[4]            ={255, 255, 255, 0};
static uint8_t staticGateway[4]         ={192, 168, 0, 10};
static char   wifiMode[4];
static char   cfgAuthor[20];
static char   cfgDate[11];
static bool    isCfgFile;
static char   cfgWifiName[64];
static char   cfgWifiPassword[64];

static EventGroupHandle_t wifiEventGroup;
static int retNum = 0;

typedef struct {
    cJSON *root;
    cJSON *settings;
    cJSON *network;
    
    bool CfgFile;
    char* mode;
    char* author;
    char* date;
    char* ipAddr;
    char* netmask;
    char* defaultGw;
} config_t;

static void Wifi_EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t Wifi_SetupConnection(void);

httpd_handle_t setup_server(void);
static esp_netif_ip_info_t connectionInfo;

extern void actualizeValue(uint8_t red, uint8_t green, uint8_t blue);

#define NOT_NUMBER      -1
#define INVALID_VALUE   -10
int paresAddrStr2Int(uint8_t* addr, const char* buffer){
    uint8_t offset = 0;
    char temp[4] = {0};
    while (buffer[offset] != '\0' && buffer[offset] != '.') {
        if (isdigit((unsigned char)buffer[offset])){
            temp[offset] = buffer[offset];
            offset++;
        }
        else {
            return NOT_NUMBER;
        }  
    }
    int value = atoi(temp);
    if (value > 255 || value < 0){
        return INVALID_VALUE;
    }
    *addr = (uint8_t*)value;
    return offset + 1;
}

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

    // TODO: change esp to be both station and router : 192.168.0.10 ! STA OR AP
    wifiEventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
#ifdef STATIC_IP_ENABLED
    /* read from config file*/
    uint8_t     fileID;
    size_t      readBytes;
    uint16_t    totalReadBytes = 0;
    char*       buffer;

    cJSON *root;
    cJSON *settings;
    cJSON *network;
    
    bool CfgFile;
    char* wifiName;
    char* wifiPassword;
    char* mode;
    char* author;
    char* date;
    char* ipAddr;
    char* netmask;
    char* defaultGw;
    char* static_ip;
    if (ESP_OK == fs_findID(&fileID)) {
        if (ESP_OK == fs_openFile(fileID, SETTING_FILE_NAME, READ_PERMISSION)) {
            buffer = malloc(1024);
            do {
                readBytes = fs_readFile(fileID, SETTING_FILE_NAME, buffer + totalReadBytes, totalReadBytes);
                totalReadBytes += readBytes;
            } while (readBytes == READ_SIZE);
            if (ESP_OK == fs_closeFile(fileID)) {
                root = cJSON_Parse(buffer);

                settings = cJSON_GetObjectItem(root, "settings");
                author = cJSON_GetObjectItem(settings, "author")->valuestring;
                date = cJSON_GetObjectItem(settings, "date")->valuestring;

                network = cJSON_GetObjectItem(settings, "network");
                wifiName = cJSON_GetObjectItem(network, "wifiName")->valuestring;
                wifiPassword = cJSON_GetObjectItem(network, "wifiPassword")->valuestring;
                ipAddr = cJSON_GetObjectItem(network, "ipAddress")->valuestring;
                netmask = cJSON_GetObjectItem(network, "netmask")->valuestring;
                defaultGw = cJSON_GetObjectItem(network, "defaultGateway")->valuestring;
                mode = cJSON_GetObjectItem(network,"STA/AP")->valuestring;
                static_ip = cJSON_GetObjectItem(network, "staticIP")->valuestring;
                CfgFile = (!strcmp(static_ip,"true")) ? true : false;
                // 1. Parsing IP.
                int offset=0;
                for (uint8_t i = 0; i < 4; i++) {
                    offset += paresAddrStr2Int(&staticIP[i], ipAddr + offset);
                    if (offset < 0) {
                        ESP_LOGE(TAG[TAG_WIFI_MODULE], "data from file corrupted! Setting default address: 192.168.0.10");
                        for (uint8_t j = 0; j < 4; j++){
                            staticIP[j] = defaultStaticIP[j];
                        }
                        break;
                    }
                }
                // 2. Parsing netmask.
                offset=0;
                for (uint8_t i = 0; i < 4; i++) {
                    offset += paresAddrStr2Int(&staticMask[i], netmask + offset);
                    if (offset < 0) {
                        ESP_LOGE(TAG[TAG_WIFI_MODULE], "data from file corrupted! Setting default mask: 255.255.255.0");
                        for (uint8_t i = 0; i < 4; i++){
                            staticMask[i] = defaultStaticMask[i];
                        }
                        break;
                    }
                }
                // 3. Parsing default gateway.
                offset=0;
                for (uint8_t i = 0; i < 4; i++) {
                    offset += paresAddrStr2Int(&staticGateway[i], defaultGw + offset);
                    if (offset < 0) {
                        ESP_LOGE(TAG[TAG_WIFI_MODULE], "data from file corrupted! Setting default gateway: 192.168.0.10");
                        for (uint8_t i = 0; i < 4; i++){
                            staticGateway[i] = defaultStaticGateway[i];
                        }
                        break;
                    }
                }
                strncpy(cfgAuthor, author, strlen(author));
                strncpy(wifiMode, mode, strlen(mode));
                strncpy(cfgDate, date, strlen(date));
                if (strlen(wifiName)>0) {
                    strncpy(cfgWifiName, wifiName, strlen(wifiName));
                }
                else {
                    strncpy(cfgWifiName, CONFIG_ESP_WIFI_SSID, strlen(CONFIG_ESP_WIFI_SSID));
                }

                if (CONFIG_ESP_WIFI_PASSWORD) {
                    strncpy(cfgWifiPassword, wifiPassword, strlen(wifiPassword));
                }
                else {
                    strncpy(cfgWifiPassword, CONFIG_ESP_WIFI_PASSWORD, strlen(CONFIG_ESP_WIFI_PASSWORD));
                }

                ESP_LOGW(TAG[0], "wifi name: %s", cfgWifiName);
                ESP_LOGW(TAG[0], "wifi passwd: %s", cfgWifiPassword);
                isCfgFile = CfgFile;
                ESP_LOGW(TAG[0], "AUTHOR: %s", cfgAuthor);
                ESP_LOGW(TAG[0], "mode: %s", wifiMode);
                ESP_LOGW(TAG[0], "date: %s", cfgDate);
            }
            else {
                ESP_LOGE(TAG[TAG_WIFI_MODULE], "File closing error.");
            }
        free(buffer);
        }
        else {
            ESP_LOGE(TAG[TAG_WIFI_MODULE], "CFG file opening error");
        }
    }
    else {
        ESP_LOGE(TAG[TAG_WIFI_MODULE], "no free file handlers");
    }
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, staticIP[0], staticIP[1], staticIP[2], staticIP[3]);
    IP4_ADDR(&ip_info.netmask, staticMask[0], staticMask[1], staticMask[2], staticMask[3]);
    IP4_ADDR(&ip_info.gw, staticGateway[0], staticGateway[1], staticGateway[2], staticGateway[3]);
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_dhcpc_stop(netif);
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
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char*)wifiConfig.sta.ssid, cfgWifiName, sizeof(wifiConfig.sta.ssid) - 1);
    strncpy((char*)wifiConfig.sta.password, cfgWifiPassword, sizeof(wifiConfig.sta.password) - 1);
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
            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_MAIN, READ_PERMISSION)) {
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
                            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_MAIN, READ_PERMISSION)) {
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
                            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_CONFIG, READ_PERMISSION)) {
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
                            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_CONTROL, READ_PERMISSION)) {
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
                /* SENDING LOGGING PAGE */
                else if (!strcmp(key, HTTP_PAGE_PARAMETER_LOGS_NAME)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_PAGE_SIZE);
                        if (pageText == NULL){
                            ESP_LOGE(TAG[1],"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTTP_PAGE_NAME_LOGS, READ_PERMISSION)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTTP_PAGE_NAME_LOGS, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG[1], "File to send: \nFile name: %s \nFile size: %d bytes", HTTP_PAGE_NAME_LOGS, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG[1], "%s page has been sent", HTTP_PAGE_NAME_LOGS);
                                    }
                                    else {
                                        ESP_LOGE(TAG[1], "%s page sending error!", HTTP_PAGE_NAME_LOGS);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG[1], "readed bytes from file: %s is equal to: %d!", HTTP_PAGE_NAME_LOGS, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG[1], "File opening error: %s", HTTP_PAGE_NAME_LOGS);
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
            ESP_LOGE(TAG[1], "Querry has not been found.");
        }
    }
    return ESP_OK;
}

/* Get information about ESP32 */
esp_err_t getInfo_EventHandler(httpd_req_t *req) {
    // const char* json_response[512]; 
    uint8_t mac[6];
    char macStr[20];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, &mac);
    snprintf(macStr, sizeof(macStr), "%x%x%x%x%x%x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    

    char ipAddrStr[16];
    snprintf(ipAddrStr, sizeof(ipAddrStr), IPSTR, IP2STR(&connectionInfo.ip));

    char netmaskStr[16];
    snprintf(netmaskStr, sizeof(netmaskStr), IPSTR, IP2STR(&connectionInfo.netmask));

    char gwStr[16];
    snprintf(gwStr, sizeof(gwStr), IPSTR, IP2STR(&connectionInfo.gw));

    char fwVStr[6];
    snprintf(fwVStr, sizeof(fwVStr), "%d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);

    char isCfgStr[6];
    snprintf(isCfgStr, sizeof(isCfgStr), "%s", isCfgFile ? "true" : "false");

    cJSON *root;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "wifiMode", cJSON_CreateString(wifiMode));
    cJSON_AddItemToObject(root, "wifiName", cJSON_CreateString(CONFIG_ESP_WIFI_SSID));
    cJSON_AddItemToObject(root, "wifiPass", cJSON_CreateString(CONFIG_ESP_WIFI_PASSWORD));
    cJSON_AddItemToObject(root, "macAddr", cJSON_CreateString(macStr)); //todo
    cJSON_AddItemToObject(root, "ipAddr", cJSON_CreateString(ipAddrStr));
    cJSON_AddItemToObject(root, "subnetMask", cJSON_CreateString(netmaskStr));
    cJSON_AddItemToObject(root, "gw", cJSON_CreateString(gwStr));
    cJSON_AddItemToObject(root, "fwV", cJSON_CreateString(fwVStr));
    cJSON_AddItemToObject(root, "cfgFile", cJSON_CreateString(isCfgStr));
    cJSON_AddItemToObject(root, "cfgAuthor", cJSON_CreateString(cfgAuthor));
    cJSON_AddItemToObject(root, "cfgTime", cJSON_CreateString(cfgDate));
 
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, cJSON_Print(root), strlen(cJSON_Print(root)));
    return ESP_OK;
}
/* CONFIGURATION FILE DOWNLOAD HANDLER */
esp_err_t getDownload_EventHandler(httpd_req_t *req) {

    char*       buffer;
    uint8_t     fileID;
    size_t      readBytes;
    uint16_t    totalReadBytes = 0;
    if (ESP_OK == fs_findID(&fileID)) {
        if (ESP_OK == fs_openFile(fileID, SETTING_FILE_NAME, READ_PERMISSION)) {
            buffer = calloc(4096, sizeof(uint8_t));
            do {
                readBytes = fs_readFile(fileID, SETTING_FILE_NAME, buffer + totalReadBytes, totalReadBytes);
                totalReadBytes += readBytes;
            } while (readBytes == READ_SIZE);
            cJSON *root = cJSON_Parse(buffer);
            if (ESP_OK == fs_closeFile(fileID)) {
                httpd_resp_set_type(req, "text/plain");
                httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"settings.json\"");
                httpd_resp_send(req, cJSON_Print(root), strlen(cJSON_Print(root)));
                ESP_LOGI(TAG[TAG_WIFI_MODULE], "Configuration file has been sent.");
            }
            else {
                ESP_LOGE(TAG[TAG_WIFI_MODULE], "Error while closing file.");
            }
            free(buffer);
        }
    }
    return ESP_OK;
}

/* TODO: handler of RGB control */
esp_err_t postRGB_EventHandler(httpd_req_t *req) {
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
        // if (red!=NULL && green!=NULL && blue!=NULL){
            httpd_resp_send(req, "Values actualized", sizeof("Values actualized"));
            actualizeValue(red, green, blue);
        // }
    }
    
    return ESP_OK;
}

esp_err_t postRGBSequence_EventHandler(httpd_req_t *req) {
    // TODO:
    httpd_resp_send(req, "Values actualized", sizeof("Values actualized"));
    return ESP_OK;
}

esp_err_t postRGBOriginal_EventHandler(httpd_req_t *req) {
    // TODO:
    httpd_resp_send(req, "Values actualized", sizeof("Values actualized"));
    return ESP_OK;
}


esp_err_t postUploadCfg_EventHandler(httpd_req_t *req) {
    // TO BE TESTED
    ESP_LOGW(TAG[TAG_WIFI_MODULE], "DUPSKO12345");
    const char* postBuffer[MAX_CFG_FILE_SIZE];
    size_t      contentLen = req->content_len;
    uint8_t     fileID;
    char        *buffer;
    buffer = malloc(contentLen);
    if (httpd_req_recv(req, buffer, contentLen) > 0){
        
        
        if (ESP_OK == fs_findID(&fileID)) {
            if (ESP_OK == fs_openFile(fileID, SETTING_FILE_NAME, WRITE_PERMISSION)) {
                ESP_LOGW(TAG[TAG_WIFI_MODULE], "received data: %s", buffer);
                size_t writtenBytes=1;
                // size_t writtenBytes = fs_writeFile(fileID, SETTING_FILE_NAME, buffer, contentLen);
                if (0 < writtenBytes) {
                    ESP_LOGW(TAG[TAG_WIFI_MODULE], "File has been send replaced!, written bytes: %d", writtenBytes);
                    
                    httpd_resp_send(req, "File saved", sizeof("File saved"));
                }
                else {
                    ESP_LOGE(TAG[TAG_WIFI_MODULE], "Amount of written bytes is different to amount of received bytres");
                }
            }
            else {
                ESP_LOGE(TAG[TAG_WIFI_MODULE], "failed to open file: %s", SETTING_FILE_NAME);
            }
            fs_closeFile(fileID);
        }
        else {
            ESP_LOGE(TAG[TAG_WIFI_MODULE], "No free file handler.");
        }
        free(buffer);
    }
    return ESP_OK;
}


esp_err_t postConfiguration_EventHandler(httpd_req_t *req) {
    size_t contentLen = req->content_len;
    size_t querryLen = httpd_req_get_url_query_len(req);
    char* querryContent;
    querryContent = malloc(contentLen);
    // eg. xxx.xxx.xxx.xxx/configuration?action=reboot
    #define URL_QUERRY_ACTION    "action"
    #define KEY_QUERRY_REBOOT    "reboot"
    #define KEY_QUERRY_SAVE      "save"       
    #define KEY_QUERRY_UPLOAD    "upload"
    
    if (ESP_OK == httpd_req_get_url_query_str(req, querryContent, querryLen + 1)){
        char key[64];
        if (ESP_OK == httpd_query_key_value(querryContent, URL_QUERRY_ACTION, key, 64)) {

            /* Handle system reboot request */
            if(0 == strcmp(key, KEY_QUERRY_REBOOT)) {
                httpd_resp_send(req, "Sys rebooted", sizeof("Sys rebooted"));
                esp_restart();
                return ESP_OK;
            }

            /* Parse received data to the configuration file */
            else if ((0 == strcmp(key, KEY_QUERRY_SAVE))){
                char *buffer;
                buffer = calloc(MAX_CFG_FILE_SIZE, sizeof(uint8_t));
                
                if (httpd_req_recv(req, buffer, contentLen) > 0){
                    char *bufferFile;
                    bufferFile = calloc(MAX_CFG_FILE_SIZE, sizeof(uint8_t));
                    size_t readBytes;
                    size_t totalReadBytes = 0;
                    uint8_t fileID;
                    if (ESP_OK == fs_findID(&fileID)) {
                        if (ESP_OK == fs_openFile(fileID, SETTING_FILE_NAME, READ_WRITE_PERMISSION)) {
                            do {
                                readBytes = fs_readFile(fileID, SETTING_FILE_NAME, bufferFile + totalReadBytes, totalReadBytes);
                                totalReadBytes += readBytes;
                            } while (readBytes == READ_SIZE);

                            config_t cfgRcv;
                            config_t cfgFile;
                            cfgRcv.root = cJSON_Parse(buffer);
                            cfgFile.root = cJSON_Parse(bufferFile);

                            cfgFile.settings = cJSON_GetObjectItem(cfgFile.root, "settings");
                            cfgFile.network = cJSON_GetObjectItem(cfgFile.settings, "network");
                            printf("1...");
                            if (NULL != cfgRcv.root){
                                printf("2...");
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "ipAddress")) {
                                    printf("3...");
                                    cJSON_GetObjectItem(cfgFile.network, "ipAddress")->valuestring = cJSON_GetObjectItem(cfgRcv.root, "ipAddress")->valuestring;
                                }
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "netmask")) {
                                    cJSON_GetObjectItem(cfgFile.network, "netmask")->valuestring = cJSON_GetObjectItem(cfgRcv.root, "netmask")->valuestring;
                                }
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "defaultGateway")) {
                                    cJSON_GetObjectItem(cfgFile.network, "defaultGateway")->valuestring = cJSON_GetObjectItem(cfgRcv.root, "defaultGateway")->valuestring;
                                }
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "staticIP")) {
                                    cJSON_GetObjectItem(cfgFile.network, "staticIP")->valuestring = cJSON_GetObjectItem(cfgRcv.root, "staticIP")->valuestring;
                                }
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "wifi_mode")) {
                                    cJSON_GetObjectItem(cfgFile.network, "STA/AP")->valuestring = cJSON_GetObjectItem(cfgRcv.root, "wifi_mode")->valuestring;
                                }
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "wifiName")) {
                                    cJSON_GetObjectItem(cfgFile.network, "wifiName")->valuestring = cJSON_GetObjectItem(cfgRcv.root, "wifiName")->valuestring;
                                }
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "wifiPassword")) {
                                    cJSON_GetObjectItem(cfgFile.network, "wifiPassword")->valuestring = cJSON_GetObjectItem(cfgRcv.root, "wifiPassword")->valuestring;
                                }
                            }
                            if (ESP_OK == fs_rewindFile(fileID)) {
                                printf("4...");
                                if (0 < fs_writeFile(fileID, SETTING_FILE_NAME, cJSON_Print(cfgFile.root), strlen(cJSON_Print(cfgFile.root)))) {
                                    printf("5...");
                                    httpd_resp_send(req, "Configuration saved", strlen("Configuration saved"));
                                }
                                else {
                                    ESP_LOGE(TAG[TAG_WIFI_MODULE], "Error while writting into configuration file: %s", SETTING_FILE_NAME);
                                }
                            }
                            else {
                                ESP_LOGE(TAG[TAG_WIFI_MODULE], "Rewinding file: %s error. Data has not been saved.", SETTING_FILE_NAME);
                            }
                            fs_closeFile(fileID);

                            free(bufferFile);
                        }
                    }
                    
                }
                free(buffer);
            }

            /* Configuration file upload & save in the LFS */
            else if ((0 == strcmp(key, KEY_QUERRY_UPLOAD))){
                char* buffer;
                buffer = malloc(MAX_CFG_FILE_SIZE);
                if (httpd_req_recv(req, buffer, contentLen) > 0){
                    httpd_resp_send(req, "File received", sizeof("File received"));

                    // JSON data must by found at multiplepart payload first to be assigned as a cJSON object.
                    char* bufferJson = strstr(buffer, "\r\n\r\n");  //find firs sequence before json data (confirmed in Wire Shark)
                    bufferJson += 4;    // Start after "\r\n\r\n"
                    char* bufferJsonEnd = strstr(buffer, "\r\n--"); //find the end of the json payload.
                    *bufferJsonEnd = '\0';  // cut out everything what is after json payload

                    cJSON *root;
                    root = cJSON_Parse(bufferJson);
                    
                    uint8_t     fileID;
                    if (ESP_OK == fs_findID(&fileID)) {
                        if (ESP_OK == fs_openFile(fileID, SETTING_FILE_NAME, WRITE_PERMISSION)) {
                            size_t writtenBytes = fs_writeFile(fileID, SETTING_FILE_NAME, cJSON_Print(root), strlen(cJSON_Print(root)));
                            if(0 < writtenBytes) {
                                ESP_LOGI(TAG[TAG_WIFI_MODULE], "%s File has been send replaced!", SETTING_FILE_NAME);
                            }
                            else {
                                ESP_LOGE(TAG[TAG_WIFI_MODULE], "Writting error: %d", writtenBytes);
                            }
                        }
                        else {
                            ESP_LOGE(TAG[TAG_WIFI_MODULE], "failed to open file: %s", SETTING_FILE_NAME);
                        }
                        fs_closeFile(fileID);
                    }
                    else {
                        ESP_LOGE(TAG[TAG_WIFI_MODULE], "No free file handler.");
                    }
                }
                free(buffer);
            }

            /* Unknown request? */
            else {
                ESP_LOGE(TAG[1], "%s is an unknown value", key);
            }
        }
        else {
            ESP_LOGE(TAG[1], "%s is not a querry key", URL_QUERRY_ACTION);
        }
    }
    else {
        ESP_LOGE(TAG[1], "Querry has not been found.");
    }
    free(querryContent);
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

httpd_uri_t get_download = {
    .uri = "/download",
    .method = HTTP_GET,
    .handler = getDownload_EventHandler,
    .user_ctx = NULL };

/* Post config */
httpd_uri_t uri_post = {
    .uri = "/RGB",
    .method = HTTP_PUT,
    .handler = postRGB_EventHandler,
    .user_ctx = NULL };

httpd_uri_t uri_post_seq = {
    .uri = "/RGB_sequence",
    .method = HTTP_PUT,
    .handler = postRGBSequence_EventHandler,
    .user_ctx = NULL };

httpd_uri_t uri_post_ori = {
    .uri = "/RGB_original",
    .method = HTTP_PUT,
    .handler = postRGBOriginal_EventHandler,
    .user_ctx = NULL };

// httpd_uri_t uri_post_setup = {
//     .uri = "/setup",
//     .method = HTTP_POST,
//     .handler = postSetup_EventHandler,
//     .user_ctx = NULL };

httpd_uri_t uri_post_uploadCfg = {
    .uri = "/upload/cfg",
    .method = HTTP_POST,
    .handler = postUploadCfg_EventHandler,
    .user_ctx = NULL };

// httpd_uri_t uri_post_sysReboot = {
//     .uri = "/sysReboot",
//     .method = HTTP_POST,
//     .handler = postSysReboot_EventHandler,
//     .user_ctx = NULL };

httpd_uri_t uri_post_configuration = {
    .uri = "/configuration",
    .method = HTTP_POST,
    .handler = postConfiguration_EventHandler,
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
        httpd_register_uri_handler(server, &uri_post_configuration);
        httpd_register_uri_handler(server, &uri_post);              //ok - to be modified with all rgb handlers included ori and seq. This will allow to reduce handlers.  
        httpd_register_uri_handler(server, &uri_post_seq);          //todo
        httpd_register_uri_handler(server, &uri_post_ori);          //todo
        httpd_register_uri_handler(server, &get_download);          //ok
        // httpd_register_uri_handler(server, &uri_post_setup);        //TODO
        httpd_register_uri_handler(server, &uri_post_uploadCfg);    // ok
        // httpd_register_uri_handler(server, &uri_post_sysReboot);    //ok
    }
    return server;
}