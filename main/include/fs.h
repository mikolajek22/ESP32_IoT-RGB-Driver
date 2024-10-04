#include "esp_littlefs.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

/*------------------------------------------
findID - returns ID of the free to use file.
fID     - OUT, ID of the free to use file handler
-------------------------------------------*/
esp_err_t findID(uint8_t *fID);

/*------------------------------------------
fs_mount - file system mounting on the devie
-------------------------------------------*/
esp_err_t fs_mount(void);

/*------------------------------------------
fs_readFile - reading the file (hardcoded size - 255 Bytes), returns amount of the read bytes
fID     - IN, ID of the free to use file handler
fName   - IN, name of the file to be read
buffer  - OUT, ptr to array where data will be stored
-------------------------------------------*/
size_t fs_readFile(uint8_t fID, char* fName, char* buffer);

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