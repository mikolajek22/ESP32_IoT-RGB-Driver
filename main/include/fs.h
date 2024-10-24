#include "esp_littlefs.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>


esp_err_t fs_openFile(uint8_t fID, char* fName);
esp_err_t fs_closeFile(uint8_t fID);
/**
 * @brief This function checks if there is free to use file handler.
 * 
 * @param fID - OUT, ptr to ID of the free to use file handler
 * 
 * @returns ESP_OK if ID has been found, else ESP_FAIL.
 **/
esp_err_t fs_findID(uint8_t *fID);

/**
 * @brief This function mounts file system on the device.
 * 
 * @returns ESP_OK if system has been mounted succesfully, else ESP_FAIL.
 **/
esp_err_t fs_mount(void);

/**
 * @brief reading from file 255 bytes or less.
 * 
 * @param fID - OUT, ptr to ID file handler
 * @param fName - IN, name of the file to be read
 * @param buffer  - OUT, ptr to array where data will be stored
 * 
 * @returns amount of readen bytes.
 **/
size_t fs_readFile(uint8_t fID, char* fName, char* buffer, size_t offest);

/*------------------------------------------
fs_writeFile - reading the file (hardcoded size - 255 Bytes), returns amount of written bytes
fID         - IN, ID of the free to use file handler
fName       - IN, name of the file to be read
writeSize   - IN, size of the data to be written into the file.
buffer      - IN, ptr to array where data will be stored
-------------------------------------------*/
size_t fs_writeFile(uint8_t fID, char* fName, char* buffer, uint16_t writeSize);

/*------------------------------------------
fs_delateFile - file system mounting on the devie
-------------------------------------------*/
esp_err_t fs_delateFile(uint8_t fID);