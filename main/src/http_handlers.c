#include "http_handlers.h"
#include "fs.h"
#include "startup.h"

/* download options */

#define KEY_QUERY_DOWNLOAD          "file"
#define VALUE_QUERY_LOGS_D          "logs"
#define VALUE_QUERY_SETIINGS_D      "settings"
#define LOG_FILE_NAME               "logs.txt"
#define SETTING_FILE_NAME           "settings.json"
/* configuration upload query */
#define KEY_QUERRY_ACTION           "action"
#define VALUE_QUERRY_REBOOT         "reboot"
#define VALUE_QUERRY_SAVE           "save"       
#define VALUE_QUERRY_UPLOAD         "upload"

/* configuration .HTML files upload query */
#define KEY_QUERY_PAGE              "page"
#define VALUE_QUERY_MAIN            "main"
#define HTML_PAGE_NAME_MAIN         "main_page.html"
#define VALUE_QUERY_CONFIG          "config"
#define HTML_PAGE_NAME_CONFIG       "config_page.html"
#define VALUE_QUERY_CONTROL         "control"
#define HTML_PAGE_NAME_CONTROL      "control_page.html"
#define VALUE_QUERY_LOGS            "logs"
#define HTML_PAGE_NAME_LOGS         "logs_page.html"

/* configuration PUT RGB query */
#define KEY_QUERY_MODE              "mode"
#define VALUE_QUERY_MANUAL          "manual"
#define VALUE_QUERY_SEQUENCE        "sequence"
#define VALUE_QUERY_ORIGINAL        "original"

#define RGB_MANUAL_MODE             0
#define RGB_AUTO_MODE               1
#define RGB_ORIGINAL_MODE           2

#define MAX_CFG_FILE_SIZE           4096        // 4  * 1024 (1kB)
#define MAX_HTML_PAGE_SIZE          24576       // 24 * 1024 (24 kB)

static const char* TAG = "http_handler";
esp_netif_ip_info_t connectionInfo;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} colors_t;

ws_info_t wsInfo = {
    .hd = NULL,
    .fd = 0,
    .isActive = false
};


extern void actualizeValue(uint8_t red, uint8_t green, uint8_t blue);
extern void actualizeMode(uint8_t redMode, uint8_t greenMode, uint8_t blueMode, uint8_t sequenceNo, uint32_t period);
extern void createSequence(colors_t first, colors_t through, colors_t last);


/* Get .html default page */
esp_err_t http_handlers_getStartPage_EventHandler(httpd_req_t *req) {
    char*       pageText;
    uint8_t     fileID;
    size_t      readBytes;
    uint16_t    totalReadBytes = 0;
    wsInfo.isActive = false;
    if (fs_findID(&fileID) == ESP_OK) { 
        pageText = malloc(MAX_HTML_PAGE_SIZE);
        if (pageText == NULL) {
            ESP_LOGE(TAG, "Not enough memory, malloc failed!");
        }
        else {
            if (ESP_OK == fs_openFile(fileID, HTML_PAGE_NAME_MAIN, READ_PERMISSION)) {
                do {
                    readBytes = fs_readFile(fileID, HTML_PAGE_NAME_MAIN, pageText + totalReadBytes, totalReadBytes);
                    totalReadBytes += readBytes;
                } while (readBytes == READ_SIZE);

                if (totalReadBytes > 0) {
                    ESP_LOGI(TAG, "File to send: File name: %s nFile size: %d bytes", HTML_PAGE_NAME_MAIN, totalReadBytes);
                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                        ESP_LOGI(TAG, "%s page has been sent", HTML_PAGE_NAME_MAIN);
                    }
                    else {
                        ESP_LOGE(TAG, "%s page sending error!", HTML_PAGE_NAME_MAIN);
                    }
                }
                else {
                    ESP_LOGE(TAG, "readed bytes from file: %s is equal to: %d!", HTML_PAGE_NAME_MAIN, totalReadBytes);
                }
                fs_closeFile(fileID);
            }
            else {
                ESP_LOGE(TAG, "File opening error: %s", HTML_PAGE_NAME_MAIN);
            }
        }
        free(pageText);
    }
    else {
        ESP_LOGE(TAG, "All file handlers are busy!");
    } 
    return ESP_OK;
}

/* Get .html pages */
esp_err_t  http_handlers_getPage_EventHandler(httpd_req_t *req) {
    char*       buffer;
    char*       pageText;
    uint8_t     fileID;
    size_t      readBytes;
    uint16_t    totalReadBytes = 0;
    size_t      bufferLen = httpd_req_get_url_query_len(req);
    wsInfo.isActive = false;
    
    if (bufferLen > 0) {
        buffer = malloc(bufferLen+1);
        memset(buffer, 0, bufferLen+1);
        if (httpd_req_get_url_query_str(req, buffer, bufferLen+1) == ESP_OK) {
            char key[64];
            //  example: 192.168.0.10/get?page=main
            if (httpd_query_key_value(buffer,KEY_QUERY_PAGE, key, 64) == ESP_OK) {
            
                /* SENDING MAIN PAGE */
                if (!strcmp(key, VALUE_QUERY_MAIN)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_HTML_PAGE_SIZE);
                        if (pageText == NULL) {
                            ESP_LOGE(TAG,"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTML_PAGE_NAME_MAIN, READ_PERMISSION)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTML_PAGE_NAME_MAIN, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG, "File to send: File name: %s File size: %d bytes", HTML_PAGE_NAME_MAIN, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG, "%s page has been sent", HTML_PAGE_NAME_MAIN);
                                    }
                                    else {
                                        ESP_LOGE(TAG, "%s page sending error!", HTML_PAGE_NAME_MAIN);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG, "readed bytes from file: %s is equal to: %d!", HTML_PAGE_NAME_MAIN, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG, "File opening error: %s", HTML_PAGE_NAME_MAIN);
                            }
                        }
                        free(pageText);
                    }
                    else {
                        ESP_LOGE(TAG, "All file handlers are busy!");
                    }
                }

                /* SENDING CONFIGURATION PAGE */
                else if (!strcmp(key, VALUE_QUERY_CONFIG)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_HTML_PAGE_SIZE);
                        if (pageText == NULL){
                            ESP_LOGE(TAG,"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTML_PAGE_NAME_CONFIG, READ_PERMISSION)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTML_PAGE_NAME_CONFIG, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG, "File to send: File name: %s File size: %d bytes", HTML_PAGE_NAME_CONFIG, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG, "%s page has been sent", HTML_PAGE_NAME_CONFIG);
                                    }
                                    else {
                                        ESP_LOGE(TAG, "%s page sending error!", HTML_PAGE_NAME_CONFIG);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG, "readed bytes from file: %s is equal to: %d!", HTML_PAGE_NAME_CONFIG, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG, "File opening error: %s", HTML_PAGE_NAME_CONFIG);
                            }
                        }
                        free(pageText);
                    }
                    else {
                        ESP_LOGE(TAG, "All file handlers are busy!");
                    }
                }

                /* SENDING CONTROL PAGE */
                else if (!strcmp(key, VALUE_QUERY_CONTROL)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_HTML_PAGE_SIZE);
                        if (pageText == NULL){
                            ESP_LOGE(TAG,"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTML_PAGE_NAME_CONTROL, READ_PERMISSION)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTML_PAGE_NAME_CONTROL, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG, "File to send: File name: %s File size: %d bytes", HTML_PAGE_NAME_CONTROL, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG, "%s page has been sent", HTML_PAGE_NAME_CONTROL);
                                    }
                                    else {
                                        ESP_LOGE(TAG, "%s page sending error!", HTML_PAGE_NAME_CONTROL);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG, "readed bytes from file: %s is equal to: %d!", HTML_PAGE_NAME_CONTROL, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG, "File opening error: %s", HTML_PAGE_NAME_CONTROL);
                            }
                        }
                        free(pageText);
                    }
                    else {
                        ESP_LOGE(TAG, "All file handlers are busy!");
                    }
                }

                /* SENDING LOGGING PAGE */
                else if (!strcmp(key, VALUE_QUERY_LOGS)) {
                    if (fs_findID(&fileID) == ESP_OK) { 
                        pageText = malloc(MAX_HTML_PAGE_SIZE);
                        if (pageText == NULL){
                            ESP_LOGE(TAG,"Not enough memory, malloc failed!");
                        }
                        else {
                            if (ESP_OK == fs_openFile(fileID, HTML_PAGE_NAME_LOGS, READ_PERMISSION)) {
                                do {
                                    readBytes = fs_readFile(fileID, HTML_PAGE_NAME_LOGS, pageText + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                if (totalReadBytes > 0) {
                                    ESP_LOGI(TAG, "File to send: File name: %s File size: %d bytes", HTML_PAGE_NAME_LOGS, totalReadBytes);
                                    if (httpd_resp_send(req, pageText, totalReadBytes) == ESP_OK) {
                                        ESP_LOGI(TAG, "%s page has been sent", HTML_PAGE_NAME_LOGS);
                                    }
                                    else {
                                        ESP_LOGE(TAG, "%s page sending error!", HTML_PAGE_NAME_LOGS);
                                    }
                                }
                                else {
                                    ESP_LOGE(TAG, "readed bytes from file: %s is equal to: %d!", HTML_PAGE_NAME_LOGS, totalReadBytes);
                                }
                            fs_closeFile(fileID);
                            }
                            else {
                                ESP_LOGE(TAG, "File opening error: %s", HTML_PAGE_NAME_LOGS);
                            }
                        }
                        free(pageText);
                    }
                    else {
                        ESP_LOGE(TAG, "All file handlers are busy!");
                    }
                }

                /* PAGE REQUEST UNKNOWN */
                else {
                    ESP_LOGE(TAG, "Invalid value of querry parameter.");
                }
            }
            else {
                ESP_LOGE(TAG, "Invalid name of querry parameter.");
            }
        }
        else {
            ESP_LOGE(TAG, "Querry has not been found.");
        }
    }
    return ESP_OK;
}

/* Response to fetch info */
esp_err_t http_handlers_getInfo_EventHandler(httpd_req_t *req) {
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
    snprintf(isCfgStr, sizeof(isCfgStr), "%s", defaultCfg.cfgFile ? "true" : "false");

    cJSON *root;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "wifiMode", cJSON_CreateString(defaultCfg.mode));
    cJSON_AddItemToObject(root, "wifiName", cJSON_CreateString(defaultCfg.wifiName));
    cJSON_AddItemToObject(root, "wifiPass", cJSON_CreateString(defaultCfg.wifiPassword));
    cJSON_AddItemToObject(root, "macAddr", cJSON_CreateString(macStr)); //todo
    cJSON_AddItemToObject(root, "ipAddr", cJSON_CreateString(ipAddrStr));
    cJSON_AddItemToObject(root, "subnetMask", cJSON_CreateString(netmaskStr));
    cJSON_AddItemToObject(root, "gw", cJSON_CreateString(gwStr));
    cJSON_AddItemToObject(root, "fwV", cJSON_CreateString(fwVStr));
    cJSON_AddItemToObject(root, "cfgFile", cJSON_CreateString(isCfgStr));
    cJSON_AddItemToObject(root, "cfgAuthor", cJSON_CreateString(defaultCfg.author));
    cJSON_AddItemToObject(root, "cfgTime", cJSON_CreateString(defaultCfg.date));
 
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, cJSON_Print(root), strlen(cJSON_Print(root)));
    return ESP_OK;
}
/* send congiuration file .json to client */
esp_err_t http_handlers_getDownload_EventHandler(httpd_req_t *req) {

    
    char*       bufferQuery;
    size_t      bufferLen = httpd_req_get_url_query_len(req);
    if (bufferLen > 0) {
        bufferQuery = malloc(bufferLen+1);
        memset(bufferQuery, 0, bufferLen+1);
        if (httpd_req_get_url_query_str(req, bufferQuery, bufferLen+1) == ESP_OK) {
            char key[64];

            //  example: 192.168.0.10/get?page=main
            if (httpd_query_key_value(bufferQuery,KEY_QUERY_DOWNLOAD, key, 64) == ESP_OK) {
                
                /* SENDING MAIN PAGE */
                if (!strcmp(key, VALUE_QUERY_SETIINGS_D)) {
                    char*       buffer;
                    uint8_t     fileID;
                    size_t      readBytes;
                    uint16_t    totalReadBytes = 0;
                    if (ESP_OK == fs_findID(&fileID)) {
                        if (ESP_OK == fs_openFile(fileID, SETTING_FILE_NAME, READ_PERMISSION)) {
                            buffer = calloc(4096, sizeof(uint8_t));
                            if (buffer == NULL) {
                                ESP_LOGE(TAG, "Not enough memory, malloc failed!");
                            }
                            else {
                                do {
                                    readBytes = fs_readFile(fileID, SETTING_FILE_NAME, buffer + totalReadBytes, totalReadBytes);
                                    totalReadBytes += readBytes;
                                } while (readBytes == READ_SIZE);
                                cJSON *root = cJSON_Parse(buffer);
                                if (ESP_OK == fs_closeFile(fileID)) {
                                    httpd_resp_set_type(req, "text/plain");
                                    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"settings.json\"");
                                    httpd_resp_send(req, cJSON_Print(root), strlen(cJSON_Print(root)));
                                    ESP_LOGI(TAG, "%s file has been sent (size %d bytes).", SETTING_FILE_NAME, totalReadBytes);
                                }
                                else {
                                    ESP_LOGE(TAG, "Error while closing file.");
                                }
                                free(buffer);
                            }  
                        }
                        else {
                            ESP_LOGE(TAG,"File opening error");
                        }
                    }
                    else {
                        ESP_LOGE(TAG,"no free file handlers");
                    }
                }
                else if (!strcmp(key, VALUE_QUERY_LOGS_D)) {
                    char*       buffer;
                    uint8_t     fileID;
                    size_t      readBytes;
                    uint16_t    totalReadBytes = 0;
                    if (ESP_OK == fs_findID(&fileID)) {
                        if (ESP_OK == fs_openFile(fileID, LOG_FILE_NAME, READ_PERMISSION)) {
                            buffer = calloc(4*1024, sizeof(uint8_t));
                            if (buffer == NULL) {
                                ESP_LOGE(TAG, "Not enough memory, malloc failed!");
                            }
                            else {
                                httpd_resp_set_type(req, "text/plain");
                                httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"logs.txt\"");
                                do {
                                    readBytes = fs_readFile(fileID, LOG_FILE_NAME, buffer, totalReadBytes);
                                    totalReadBytes += readBytes;
                                    httpd_resp_send_chunk(req, buffer, readBytes);
                                } while (readBytes == READ_SIZE);
                                httpd_resp_send_chunk(req, NULL, 0);
                                if (ESP_OK == fs_closeFile(fileID)) {
                                    ESP_LOGI(TAG, "%s file has been sent (size %d bytes).", LOG_FILE_NAME, totalReadBytes);
                                }
                                else {
                                    ESP_LOGE(TAG, "Error while closing file.");
                                } 
                            }
                            free(buffer);
                        }
                        else {
                            ESP_LOGE(TAG,"File opening error");
                        }
                    }
                    else {
                        ESP_LOGE(TAG,"no free file handlers");
                    }
                }
                else {
                    ESP_LOGE(TAG,"Unknown Value");
                }
            }
            else {
                ESP_LOGE(TAG,"Unknown Key");
            }
        }
        free(bufferQuery);
    }
    return ESP_OK;
}

/* TODO: handler of RGB control */
esp_err_t http_handlers_postRGB_EventHandler(httpd_req_t *req) {

    colors_t manualRGB;
    uint8_t sequenceNo = 0;
    colors_t originalRGB[3];
    uint16_t originalPeriod;

    char*       buffer;
    size_t      bufferLen = httpd_req_get_url_query_len(req);
    char*       bufferData;
    size_t      dataLen = req->content_len;
    bufferData = calloc(MAX_CFG_FILE_SIZE, sizeof(uint8_t));
    if (bufferLen > 0) {
        buffer = malloc(bufferLen + 1);
        memset(buffer, 0, bufferLen + 1);
        if (httpd_req_get_url_query_str(req, buffer, bufferLen + 1) == ESP_OK) {
            char key[64];
            //  example: 192.168.0.10/RGB?mode=manual
            if (httpd_query_key_value(buffer, KEY_QUERY_MODE, key, 64) == ESP_OK) {
            
                /* RECEIVING MANUAL */
                if (!strcmp(key, VALUE_QUERY_MANUAL)) {
                    if (httpd_req_recv(req, bufferData, dataLen) > 0) { 
                        cJSON *root = cJSON_Parse(bufferData);
                        cJSON *rgbValues = cJSON_GetObjectItem(root, "rgbValues");
                        manualRGB.red = cJSON_GetObjectItem(rgbValues, "red")->valueint;
                        manualRGB.green = cJSON_GetObjectItem(rgbValues, "green")->valueint;
                        manualRGB.blue = cJSON_GetObjectItem(rgbValues, "blue")->valueint;
                        actualizeValue(manualRGB.red, manualRGB.green, manualRGB.blue);
                        actualizeMode(RGB_MANUAL_MODE, RGB_MANUAL_MODE, RGB_MANUAL_MODE, 0, 2000);
                        httpd_resp_send(req, "Values actualized", sizeof("Values actualized"));
                        
                    }
                }

                /* RECEIVING SEQUENCE */
                else if (!strcmp(key, VALUE_QUERY_SEQUENCE)) {
                    if (httpd_req_recv(req, bufferData, dataLen) > 0) { 
                        cJSON *root = cJSON_Parse(bufferData);
                        cJSON *rgbValues = cJSON_GetObjectItem(root, "sequence");
                        sequenceNo = cJSON_GetObjectItem(rgbValues, "number")->valueint;
                        actualizeMode(RGB_AUTO_MODE, RGB_AUTO_MODE, RGB_AUTO_MODE, sequenceNo, 2000);
                        httpd_resp_send(req, "Mode changed", sizeof("Mode changed"));
                    }
                }

                /* RECEIVING ORIGINAL */
                // TODO!!!!!
                else if (!strcmp(key, VALUE_QUERY_ORIGINAL)) {
                    if (httpd_req_recv(req, bufferData, dataLen) > 0) { 
                        cJSON *root = cJSON_Parse(bufferData);
                        cJSON *sequence = cJSON_GetObjectItem(root, "sequence");
                        cJSON *sFirst = cJSON_GetObjectItem(sequence, "first");
                        originalRGB[0].red = cJSON_GetObjectItem(sFirst, "red")->valueint;
                        originalRGB[0].green = cJSON_GetObjectItem(sFirst, "green")->valueint;
                        originalRGB[0].blue = cJSON_GetObjectItem(sFirst, "blue")->valueint;

                        cJSON *sSecond = cJSON_GetObjectItem(sequence, "second");
                        originalRGB[1].red = cJSON_GetObjectItem(sFirst, "red")->valueint;
                        originalRGB[1].green = cJSON_GetObjectItem(sFirst, "green")->valueint;
                        originalRGB[1].blue = cJSON_GetObjectItem(sFirst, "blue")->valueint;

                        cJSON *sThird = cJSON_GetObjectItem(sequence, "third");
                        originalRGB[2].red = cJSON_GetObjectItem(sFirst, "red")->valueint;
                        originalRGB[2].green = cJSON_GetObjectItem(sFirst, "green")->valueint;
                        originalRGB[2].blue = cJSON_GetObjectItem(sFirst, "blue")->valueint;

                        cJSON *sTime = cJSON_GetObjectItem(sequence, "time");
                        originalPeriod = cJSON_GetObjectItem(sFirst, "period")->valueint;

                        actualizeValue(0, 0, 0);
                        actualizeMode(RGB_ORIGINAL_MODE, RGB_ORIGINAL_MODE, RGB_ORIGINAL_MODE, 0, 2000);
                        createSequence(originalRGB[0], originalRGB[1], originalRGB[2]);
                        httpd_resp_send(req, "Original On", sizeof("Original On"));
                    }  
                }

                /* UNKNOWN VALUE */
                else {
                    ESP_LOGE(TAG, "Unknown query value");
                }
            }
            else {
                ESP_LOGE(TAG, "Unknown query key");
            }
        }
        else {
            ESP_LOGE(TAG, "Query string not found");
        }
        free(buffer);
    }
    else {
        ESP_LOGE(TAG, "Buffer Length is 0.");
    }
    free(bufferData);
    
    return ESP_OK;
}


esp_err_t http_handlers_postConfiguration_EventHandler(httpd_req_t *req) {
    size_t contentLen = req->content_len;
    size_t querryLen = httpd_req_get_url_query_len(req);
    char* querryContent;
    querryContent = malloc(contentLen);
    // eg. xxx.xxx.xxx.xxx/configuration?action=reboot
    
    if (ESP_OK == httpd_req_get_url_query_str(req, querryContent, querryLen + 1)){
        char key[64];
        if (ESP_OK == httpd_query_key_value(querryContent, KEY_QUERRY_ACTION, key, 64)) {

            /* Handle system reboot request */
            if(0 == strcmp(key, VALUE_QUERRY_REBOOT)) {
                httpd_resp_send(req, "Sys rebooted", sizeof("Sys rebooted"));
                esp_restart();
                return ESP_OK;
            }

            /* Parse received data to the configuration file */
            else if ((0 == strcmp(key, VALUE_QUERRY_SAVE))){
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

                            espConfigurationFile_t cfgRcv;
                            espConfigurationFile_t cfgFile;
                            cfgRcv.root = cJSON_Parse(buffer);
                            cfgFile.root = cJSON_Parse(bufferFile);

                            cfgFile.settings = cJSON_GetObjectItem(cfgFile.root, "settings");
                            cfgFile.network = cJSON_GetObjectItem(cfgFile.settings, "network");
                            if (NULL != cfgRcv.root){
                                if (NULL != cJSON_GetObjectItem(cfgRcv.root, "ipAddress")) {
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
                            if (ESP_OK == fs_rewindFile(fileID, true)) {
                                ESP_LOGW(TAG,"saving file :%s",cJSON_Print(cfgFile.root));
                                if (0 < fs_writeFile(fileID, SETTING_FILE_NAME, cJSON_Print(cfgFile.root), strlen(cJSON_Print(cfgFile.root)))) {
                                    httpd_resp_send(req, "Configuration saved", strlen("Configuration saved"));
                                }
                                else {
                                    ESP_LOGE(TAG, "Error while writting into configuration file: %s", SETTING_FILE_NAME);
                                }
                            }
                            else {
                                ESP_LOGE(TAG, "Rewinding file: %s error. Data has not been saved.", SETTING_FILE_NAME);
                            }
                            fs_closeFile(fileID);

                            free(bufferFile);
                        }
                    }
                    
                }
                free(buffer);
            }

            /* Configuration file upload & save in the LFS */
            else if ((0 == strcmp(key, VALUE_QUERRY_UPLOAD))){
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
                                ESP_LOGI(TAG, "%s File has been send replaced!", SETTING_FILE_NAME);
                            }
                            else {
                                ESP_LOGE(TAG, "Writting error: %d", writtenBytes);
                            }
                        }
                        else {
                            ESP_LOGE(TAG, "failed to open file: %s", SETTING_FILE_NAME);
                        }
                        fs_closeFile(fileID);
                    }
                    else {
                        ESP_LOGE(TAG, "No free file handler.");
                    }
                }
                free(buffer);
            }

            /* Unknown request? */
            else {
                ESP_LOGE(TAG, "%s is an unknown value", key);
            }
        }
        else {
            ESP_LOGE(TAG, "%s is not a querry key", KEY_QUERRY_ACTION);
        }
    }
    else {
        ESP_LOGE(TAG, "Querry has not been found.");
    }
    free(querryContent);
    return ESP_OK;
}


void http_handlers_sendOverWS(const char* buffer){

    if (!wsInfo.isActive || !wsInfo.hd) {
        return;
    }
    else {
        httpd_ws_frame_t wk_pkt = {
            .type = HTTPD_WS_TYPE_TEXT,
            .len = strlen(buffer),
            .payload = (uint8_t*)buffer
        };
        if (ESP_OK != httpd_ws_send_frame_async(wsInfo.hd, wsInfo.fd, &wk_pkt)) {
            wsInfo.isActive = false;
        }
    }
    
}

esp_err_t http_handlers_websocketEnable_EventHandler(httpd_req_t *req) {

    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        wsInfo.hd = req->handle;
        wsInfo.fd = httpd_req_to_sockfd(req);
        wsInfo.isActive = true;
        return ESP_OK;
    }
    else {
        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.payload = NULL;

        esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
        // WS Connection close
        if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
            ESP_LOGI(TAG, "Client closed WebSocket connection");
            wsInfo.isActive = false;
        }
        return ESP_OK;
    }
}
