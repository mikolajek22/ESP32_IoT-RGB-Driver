#include "startup.h"
#include "fs.h"
#include "ctype.h"

#define CONFIG_ESP_WIFI_SSID        "ERROR"        
#define CONFIG_ESP_WIFI_PASSWORD    "ERROR"  

#define SETTING_FILE_NAME           "settings.json"

/* parser */
#define PARSER_NOT_NUMBER      -1
#define PARSER_INVALID_VALUE   -10

static const char *TAG = "Startup";

static const uint8_t defaultIpAddr[4]   = {192, 168, 0, 10};
static const uint8_t defaultMaskAddr[4] = {255, 255, 255, 0};
static const uint8_t defaultGwAddr[4]   = {192, 168, 0, 10};

espConfiguration_t defaultCfg;

int paresAddrStr2Int(uint8_t* addr, const char* buffer);

esp_err_t startup_ReadConfiguration() {
    uint8_t     fileID;
    size_t      readBytes      = 0;
    uint16_t    totalReadBytes = 0;
    char*       buffer;

    if (ESP_OK == fs_findID(&fileID)) {
        if (ESP_OK == fs_openFile(fileID, SETTING_FILE_NAME, READ_PERMISSION)) {
            buffer = malloc(1024);
            do {
                readBytes = fs_readFile(fileID, SETTING_FILE_NAME, buffer + totalReadBytes, totalReadBytes);
                totalReadBytes += readBytes;
            } while (readBytes == READ_SIZE);

            if (ESP_OK == fs_closeFile(fileID)) {
                espConfigurationFile_t defaultCfgJson;
                defaultCfgJson.root = cJSON_Parse(buffer);
                defaultCfgJson.settings     = cJSON_GetObjectItem(defaultCfgJson.root, "settings");

                defaultCfgJson.author       = cJSON_GetObjectItem(defaultCfgJson.settings, "author")->valuestring;
                defaultCfgJson.date         = cJSON_GetObjectItem(defaultCfgJson.settings, "date")->valuestring;
                defaultCfgJson.network      = cJSON_GetObjectItem(defaultCfgJson.settings, "network");

                defaultCfgJson.wifiName     = cJSON_GetObjectItem(defaultCfgJson.network, "wifiName")->valuestring;
                defaultCfgJson.wifiPassword = cJSON_GetObjectItem(defaultCfgJson.network, "wifiPassword")->valuestring;
                defaultCfgJson.ipAddr       = cJSON_GetObjectItem(defaultCfgJson.network, "ipAddress")->valuestring;
                defaultCfgJson.netmask      = cJSON_GetObjectItem(defaultCfgJson.network, "netmask")->valuestring;
                defaultCfgJson.defaultGw    = cJSON_GetObjectItem(defaultCfgJson.network, "defaultGateway")->valuestring;
                defaultCfgJson.mode         = cJSON_GetObjectItem(defaultCfgJson.network,"STA/AP")->valuestring;
                defaultCfgJson.staticIP     = cJSON_GetObjectItem(defaultCfgJson.network, "staticIP")->valuestring;
                defaultCfg.cfgFile = (!strcmp(defaultCfgJson.staticIP, "true")) ? true : false;
                // 1. Parsing IP.
                int offset = 0;
                for (uint8_t i = 0; i < 4; i++) {
                    offset += paresAddrStr2Int(&defaultCfg.ipAddr[i], defaultCfgJson.ipAddr + offset);
                    if (offset < 0) {
                        ESP_LOGE(TAG, "data from file corrupted! Setting default address: 192.168.0.10");
                        for (uint8_t j = 0; j < 4; j++){
                            defaultCfg.ipAddr[j] = defaultIpAddr[j];
                        }
                        break;
                    }
                }

                // 2. Parsing netmask.
                offset = 0;
                for (uint8_t i = 0; i < 4; i++) {
                    offset += paresAddrStr2Int(&defaultCfg.netmask[i], defaultCfgJson.netmask + offset);
                    if (offset < 0) {
                        ESP_LOGE(TAG, "data from file corrupted! Setting default mask: 255.255.255.0");
                        for (uint8_t i = 0; i < 4; i++){
                            defaultCfg.netmask[i] = defaultMaskAddr[i];
                        }
                        break;
                    }
                }

                // 3. Parsing default gateway.
                offset = 0;
                for (uint8_t i = 0; i < 4; i++) {
                    offset += paresAddrStr2Int(&defaultCfg.defaultGw[i], defaultCfgJson.defaultGw + offset);
                    if (offset < 0) {
                        ESP_LOGE(TAG, "data from file corrupted! Setting default gateway: 192.168.0.10");
                        for (uint8_t i = 0; i < 4; i++){
                            defaultCfg.defaultGw[i] = defaultGwAddr[i];
                        }
                        break;
                    }
                }
                strncpy(defaultCfg.author, defaultCfgJson.author, strlen(defaultCfgJson.author));
                strncpy(defaultCfg.mode, defaultCfgJson.mode, strlen(defaultCfgJson.mode));
                strncpy(defaultCfg.date, defaultCfgJson.date, strlen(defaultCfgJson.date));
                if (strlen(defaultCfgJson.wifiName) > 0) {
                    strncpy(defaultCfg.wifiName, defaultCfgJson.wifiName, strlen(defaultCfgJson.wifiName));
                }
                else {
                    strncpy(defaultCfg.wifiName, CONFIG_ESP_WIFI_SSID, strlen(CONFIG_ESP_WIFI_SSID));
                }

                if (CONFIG_ESP_WIFI_PASSWORD) {
                    strncpy(defaultCfg.wifiPassword, defaultCfgJson.wifiPassword, strlen(defaultCfgJson.wifiPassword));
                }
                else {
                    strncpy(defaultCfg.wifiPassword, CONFIG_ESP_WIFI_PASSWORD, strlen(CONFIG_ESP_WIFI_PASSWORD));
                }
                
                ESP_LOGI(TAG, "IP ADDRESS: %d.%d.%d.%d", defaultCfg.ipAddr[0], defaultCfg.ipAddr[1], defaultCfg.ipAddr[2], defaultCfg.ipAddr[3]);
                ESP_LOGI(TAG, "WIFI SSID: %s", defaultCfg.wifiName);
                ESP_LOGI(TAG, "WIFI PASSWORD: %s", defaultCfg.wifiPassword);
                ESP_LOGI(TAG, "MODE: %s", defaultCfg.mode);
            }
            else {
                ESP_LOGE(TAG, "File closing error.");
            }
        free(buffer);
        }
        else {
            ESP_LOGE(TAG, "CFG file opening error");
        }
    }
    else {
        ESP_LOGE(TAG, "no free file handlers");
    }
    return ESP_OK;
}


int paresAddrStr2Int(uint8_t* addr, const char* buffer){
    uint8_t offset  = 0;
    char temp[4]    = {0};

    while (buffer[offset] != '\0' && buffer[offset] != '.') {
        if (isdigit((unsigned char)buffer[offset])){
            temp[offset] = buffer[offset];
            offset++;
        }
        else {
            return PARSER_NOT_NUMBER;
        }  
    }

    int value = atoi(temp);
    if (value > 255 || value < 0) {
        return PARSER_INVALID_VALUE;
    }
    *addr = (uint8_t*)value;
    return offset + 1;
}