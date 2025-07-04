#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "cJSON.h"
#include "stdbool.h"

/**
 * @note Struct that contains all information from configuration file - including network configuration.
 **/
typedef struct {
    cJSON *root;

    cJSON *settings;

    char* author;
    char* date;
    cJSON *network;
    
    char* wifiName;
    char* wifiPassword;
    char* ipAddr;
    char* defaultGw;
    char* netmask;
    char* staticIP;
    char* mode;
} espConfigurationFile_t;

typedef struct {
    char  author[20];
    char date[11];
    char wifiName[64];
    char wifiPassword[64];

    uint8_t ipAddr[4];
    uint8_t defaultGw[4];
    uint8_t netmask[4];
    bool  staticIP;
    bool  cfgFile;
    char  mode[4];
} espConfiguration_t;

extern espConfiguration_t defaultCfg;

/**
 * @brief called only once during starting up main app. Reading configuration file and setting up logs
 * 
 * @returns ESP_OK if configuration done properly, else ESP_FAIL.
 **/
esp_err_t startup_ReadConfiguration();