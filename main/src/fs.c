/*----------------------------------
author: mikolajek :)
this file is used to handle the little file system.
there is possibility to open maximum amount of the files equal to 6 at the same time.
at this project there will be only one file used in the moment - html file to view webserver. In futhure there will be possibility to read
G-code files and .conf files at the same time.

last update: 04.10.2024
-----------------------------------*/

#include "fs.h"
#include "freertos/FreeRTOS.h"

#define FILE_OPENED             1
#define FILE_CLOSED             0

#define READ_SIZE               4096

static const char *LOG_TAG = "FILE_SYS";
static esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "littlefs",
        .format_if_mount_failed = true,
        .dont_mount = false
};
typedef struct {
    FILE* file;
    uint8_t id;
    bool open;
}fileID;

static fileID arrFiles[6];
static SemaphoreHandle_t fileMutex;

esp_err_t fs_openFile(uint8_t fID, char* fName, char* permission) {
    xSemaphoreTake(fileMutex, pdMS_TO_TICKS(500));
    esp_err_t ret;
    char fileName[255] = "/littlefs/";
    strcat(fileName,fName);
    if (arrFiles[fID].open == FILE_CLOSED){
        arrFiles[fID].file = fopen(fileName, permission);
        arrFiles[fID].open = FILE_OPENED;
        // ESP_LOGI(LOG_TAG, "File opened!");
        ret = ESP_OK;
    }
    else {
        // ESP_LOGE(LOG_TAG, "File opening error - file already opened!");
        ret = ESP_FAIL;
    }
    xSemaphoreGive(fileMutex);
    return ret;
}
esp_err_t fs_closeFile(uint8_t fID) {
    xSemaphoreTake(fileMutex, pdMS_TO_TICKS(500));
    esp_err_t ret;
    if (arrFiles[fID].open == FILE_OPENED){
        fclose(arrFiles[fID].file);
        arrFiles[fID].open = FILE_CLOSED;
        // ESP_LOGI(LOG_TAG, "File closed!");
        ret = ESP_OK;
    }
    else {
        // ESP_LOGE(LOG_TAG, "File already closed");
        ret = ESP_FAIL;
    }
    xSemaphoreGive(fileMutex);
    return ret;
}

// Mounting file sys
esp_err_t fs_mount(void){
    ESP_LOGI(LOG_TAG, "Initializing Little File System");
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret == ESP_OK){
        fileMutex = xSemaphoreCreateMutex();
        ESP_LOGI(LOG_TAG, "Mounting Completed");
    } 
    else if (ret == ESP_ERR_NOT_FOUND){
         ESP_LOGE(LOG_TAG, "Partition not found!");
    }
    else {
        ESP_LOGE(LOG_TAG, "Mounting Failed!");
    }
    return ret;
}

size_t fs_readFile(uint8_t fID, char* fName, char* buffer, size_t offest){
    xSemaphoreTake(fileMutex, pdMS_TO_TICKS(500));
    size_t ret;
    size_t readBytes;
    if (arrFiles[fID].file != NULL){
        readBytes = fread(buffer, 1, READ_SIZE, arrFiles[fID].file);
        ret = readBytes;
    }
    else {
        // ESP_LOGE(LOG_TAG, "File is null!");
         ret = ESP_FAIL;
    }
    xSemaphoreGive(fileMutex);
    return ret;
}


size_t fs_writeFile(uint8_t fID, char* fName, char* buffer, uint16_t writeSize){
    xSemaphoreTake(fileMutex, pdMS_TO_TICKS(500));
    size_t ret;
    size_t writtenBytes;
    if (arrFiles[fID].file != NULL){
        // arrFiles[fID].open = FILE_OPENED;
        writtenBytes = fwrite(buffer, 1, writeSize, arrFiles[fID].file);
        ret = writtenBytes;
    } 
    else {
        // ESP_LOGE(LOG_TAG, "write file File is null!");
        ret = ESP_FAIL;
    }
    xSemaphoreGive(fileMutex);
    return ret;
}

esp_err_t fs_rewindFile(uint8_t fID, bool overwrite) {
    xSemaphoreTake(fileMutex, pdMS_TO_TICKS(500));
    if (overwrite) {ftruncate(fileno(arrFiles[fID].file), 0);}   //find file decription, set length to 0. This will cause owerwritting whole file.
    if (arrFiles[fID].file != NULL){ 
        rewind(arrFiles[fID].file);
        xSemaphoreGive(fileMutex);
        return ESP_OK;
    }
    else {
        xSemaphoreGive(fileMutex);
        return ESP_FAIL;
    }
}

esp_err_t fs_delateFile(uint8_t fID){
    return ESP_OK;
}

esp_err_t fs_findID(uint8_t *fID){
    xSemaphoreTake(fileMutex, pdMS_TO_TICKS(500));
    for(uint8_t i = 0; i < 6; i++){
        if (arrFiles[i].open == FILE_CLOSED){
            arrFiles[i].id = i;
            *fID = i;
            xSemaphoreGive(fileMutex);
            return ESP_OK;
        }
    }
    xSemaphoreGive(fileMutex);
    return ESP_FAIL;
}

size_t fs_fileSize(uint8_t fID) {
    FILE *file = arrFiles[fID].file;
    if (file == NULL) {
        return 0;
    }
    long currentPos = ftell(file);
    fseek(file, 0, SEEK_END); 
    long size = ftell(file);
    fseek(file, currentPos, SEEK_SET);

    return (size_t)size;
}