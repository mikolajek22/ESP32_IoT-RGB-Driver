# IoT RGB driver
This is a project of RGB driver based on ESP32 WROOM32u devkit C.

## Application description
ESP32 is used to control 12V RGB strip via 3 PWM ports. Electrical diagram with used components can be also found on my Google Disc (Link can be found below). Some parts of the application could have been written in a simpler way - I wanted to learn some new thing so this and test how does everything works so it might be sometimes overcomplcated. It is also not neccessery to mix C++ with C. It was done because of my curiosity if I can do it and if it will be even working on ESP32.

### Hardware components

### Software description
#### Configuration
ESP32 is a HTTP server. The default network settings are hardcoded in case of sending incorrect configuration file ("settings.json). At this moment network setting can be changed via settings.json only. File can be downloaded from esp32 and uploaded to esp32 via web server. Another way to upload file to flash is using command in ESP-IDF CMD
```bash
idf.py partition_table littlefs-flash
```
this command will write all the files that are included in pages_html directory (this can be changed in CMakeList.txt). Third way to change network settings is to set them directly on http server, on the configuration page. Anyway after changing network settings, esp32 must be rebooted. Reboot can be performed via htpp (recommended) or just by removing power supply.
The Ip address can be verified also on the oled display but at this moment there is no feature of changing it.

#### HTTP Server
SSID and password of the WiFi is set in settings.json. While booting, file is readed and esp tries to connect to he wifi using this settings. In case of faile esp will retry to connect to the AP in 5s, next 10s, 15s ... up to 30s, then every 30 seconds. Server is accesible via web browser by typing in browser IP addres. Soon there will be added AP as a default configuration of esp32. This will enable to configure network without changing the software.
1. Home:
    Basic Informations can be readed from main page. Configuration file also can be downloada at this page.
2. Configuration:
    Configuration page allows to upload new configuration file (you need to send settings.json), or change settings directly from the level of the client browser and click Save settings. After changing configuration you can reboot device there
3. RGB CONTROLLER:
    change values of physical RGB strip - will be described below in "DRIVER"
4. LOGS:
    Logs page allows to track logs from esp in real time. Logs are send via WS. You can also download logging file from the device. Maximum size of the logging file is up to 512 kB so you cane see history of logging.
#### LOGGING
In general logs can be readed from via UART.
```bash
idf.py -p COMx monitor
```
Logs are also redirected to the http server, using the web socket. In that case logs are displayed in real time. Every 5 second ESP sends diagnostic status where is described RAM usage, date and time.

#### Driver
Colors can be set from "RGB Controller" page. 
1. Manual Mode:
    In manual mode you can choose color form RGB picker or by pressing one button of the five main colors. If you want to create your own color you can use color box that and select the color using cursor. 
2. Sequence Mode:
    There is also auto-mode which allow to light in sequences. There are 4 default sequences (0 - 3) that are actually coded inside the "rgbController.cpp". 
3. Original Mode:
    There is also possibility to create your own sequence fromm http website interface. It is named Original Sequence. In that case you shoud set the 3 base values that will be included in the sequence. The sequence will start from the first base, fading through second base to third base. Then again driver is fading from third to the first base. At this moment there are always 3 bases, even if you will not fill the third base, sequence will fade to zero values at the last base. 
4. Period Time:
    You can also set the period time - it has impact both on the hardcoded sequences and original sequences:
    How to calculate time? This time refers to delay between each iteration. Min value and Max value of each color is 255 (8 bit). If period is set to 100 ms then full period of changed color will take (255 * 100) ms.
5. Original Sequence Speed:
    Original sequence speed can be set (val 1 - 100). It defines the increment factor for each iteration. If speed is set to 100% and sequence goes fro R255 to 0, with period set to 100ms, that means the red value will be decreased from 255 to 0 in 100 ms. If in the same case speed will be set to then the red value will be decreased from 255 to 0 in (255 * 100) ms.

#### File system
All .html files (and settings.json file) are stored in flash file system - little fs. The LittleFS library is included into this project and can be also found here:
https://github.com/joltwallet/esp_littlefs
At this moment, there is possibility to have opened maximum number of 6 files at the same time. The interface of the file system is well descrbed directly in the file "fs.h"

#### Keyboard and Oled
Threre are 5 buttons and OLED screen. It works as a clock that is synchronized with SNTP. Using keyboard you can check device's IP, Netmask and reboot it.

#### Led Diods
There are 2 leds, Green and Red. Green is blinking every seconds. It indicates that system works. Red Diod blinks if the button has been pushed.

#### Power supply
To supply RGB strip I am using 60W 12V DC Power supply. Inside the device there is Step-down from 12 to 3.3V to power up ESP32. Used step-down converter works up to 36V so you can use RGB strip with higher voltage. Remember to set up properly output of the step-down converter in order to protect ESP32.

#### Links
Electrical Scheme: 
https://drive.google.com/drive/folders/1yBjRe1mOAP0S1CEnbVP0L3ItTyetpFwV?usp=sharing
Mechanical Scheme (this one is actually a crap - you can try it for fun but I recommend to make it better):
https://drive.google.com/drive/folders/1x4RYR9BXcr16XOpKlXQYmXoEfsy7SJZZ?usp=sharing

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


### debugging
There are JTAG ping so it is possible to debug device using ESP-PROG. The example of VSCode Configuration is in .vscode. There is nice YouTube tutrial:
https://www.youtube.com/watch?app=desktop&v=uq93H7T7cOQ - it is nice described step by step.
### TODO:
* BLE server to set network configuration in case of problems with internet connection.
* BLE server to controll rgb strip.
* Configuration file upload over UART.

