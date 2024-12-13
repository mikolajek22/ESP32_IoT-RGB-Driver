# IoT RGB driver
This is a project of RGB driver based on ESP32 WROOM32u devkit C.

## Application description
ESP32 is used to control 12V RGB strip via 3 PWM ports. Electrical diagram with used components can be also found on my github in forder /diagrams. Some parts of the application could have been written in a simpler way. It is also not neccessery to mix C++ with C. It was done because of my curiosity if I can do it and if it will be even working on ESP32.

### Hardware components

### Software description
#### Configuration
ESP32 is a HTTP server. The default network settings are hardcoded in case of sending incorrect configuration file ("settings.json). At this moment network setting can be changed via settings.json. File can be downloaded from esp32 and uploaded to esp32 via web server. Another way to upload file to flash is using command in ESP-IDF CMD
```bash
idf.py partition_table littlefs-flash
```
this command will write all the files that are included in pages_html directory (this can be changed in CMakeList.txt). Third way to change network settings is to set them directly on http server, on the configuration page. Anyway after changing network settings, esp32 must be rebooted. Reboot can be performed via htpp (recommended) or just by removing power supply.
#### Driver
Colors can be set from "RGB Controller" page. In manual mode you can choose color form RGB picker or by pressing one button of the five main colors. There is also auto-mode which allow to light in sequences. There are 4 default sequences (0 - 3) that are actually coded inside the "rgbController.cpp". There is also possibility to create your own sequence fromm http website interface. It is named Original Sequence. In that case you shoud set the 3 base values that will be included in the sequence. The sequence will start from the first base, fading through second base to third base. Then again driver is fading from third to the first base. There is also possibility to set period of time between next base points.
#### File system
All .html files (and settings.json file) are stored in flash file system - little fs. The LittleFS library is included into this project and can be also found here:
https://github.com/joltwallet/esp_littlefs
At this moment, there is possibility to have opened maximum number of 6 files at the same time. The interface of the file system is well descrbed directly in the file "fs.h"
#### HTTP Server
SSID and password of the WiFi is set in settings.json. While booting, file is readed and esp tries to connect to he wifi using this settings. In case of faile esp will retry to connect to the AP in 5s, next 10s, 15s ... up to 30s, then every 30 seconds. Server is accesible via web browser by typing in browser IP addres.
Basic Informations can be readed from main page. Configuration file also can be downloada at this page.
Configuration page allows to upload new configuration file, or change settings directly from the level of the client browser.
RGB Controller - change values of physical color PINS.
Logs - Logs page allows to track logs from esp in real time. Logs are send via WS.
#### LOGGING
In general logs can be readed from via UART.
```bash
idf.py -p COMx monitor
```
Logs are also redirected to the http server, using the web socket. In that case logs are displayed in real time.

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
* Configuration file upload over UART.

