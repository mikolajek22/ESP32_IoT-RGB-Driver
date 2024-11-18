# IoT RGB driver
This is a project of RGB driver based on ESP32 WROOM32u devkit C.

## Application description
ESP32 is used to control 12V RGB strip via 3 PWM ports. Electrical diagram with used components can be also found on my github in forder /diagrams. Some parts of the application could have been written in a simpler way. It is also not neccessery to mix C++ with C. It was done because of my curiosity if I can do it and if it will be even working on ESP32.

### Hardware components

### Software
ESP32 is a HTTP server. The default network settings are hardcoded in case of sending incorrect configuration file ("settings.json). At this moment network setting can be changed via settings.json. File can be downloaded from esp32 and uploaded to esp32 via web server. Another way to upload file to flash is using command in ESP-IDF CMD
```bash
idf.py partition_table littlefs-flash
```
this command will write all the files that are included in pages_html directory (this can be changed in CMakeList.txt). Third way to change network settings is to set them directly on http server, on the configuration page. Anyway after changing network settings, esp32 must be rebooted. 
Colors can be set from "RGB Controller" page. In manual mode you can choose color form RGB picker or by pressing one button of the five main colors. There is also auto-mode with allow to light in sequences. There are 4 default sequences that are actually coded inside the "rgbController.cpp". There is also possibility to create your own sequence fromm http website. In that case you shoud set the 3 values that will be included in the sequence. There is also possibility to set period of time between next points.
All .html files (and settings.json file) are stored in flash file system - little fs. The LittleFS library is included into this project and can be also found here:
https://github.com/joltwallet/esp_littlefs
On the Logs page there is possibility to read loggings from esp32. Esp will be sending logs after enabling "start logging" button.

### flashing step by step
1. Run ESP-IDF CMD (in my case v5.3.1.).
2. Build the project:
```bash
idf.py build
```
3. Flash all files to the flash:
```bash
idf.py partition_table littlefs-flash
```
4. Flash the application (X - number of serial port eg. COM3):
```bash
idf.py -p COMX flash
```
5. Logs monitoring (optional):
```bash
idf.py -p COMX monitor
```
### TODO:
* BLE server to set network configuration in case of problems with internet connection.
* BLE server to controll rgb strip.
# TO BE DELETED: 
command:
idf.py partition_table littlefs_flash

