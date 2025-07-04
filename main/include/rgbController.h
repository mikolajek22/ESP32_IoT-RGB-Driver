#include <cstdlib>
#include <iostream>
#include <thread>
#include "driver/ledc.h"

#define RGB_MANUAL_MODE             0
#define RGB_AUTO_MODE               1
#define RGB_ORIGINAL_MODE           2
std::mutex mtxVal;
std::mutex mtxMod;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} colors_t;

extern "C"{
    /* this functions are called from http_server.c! */

    /**
     * @brief Moving values from http server to hardware RGB driver.
     * 
     * @param red red value
     * @param green green value
     * @param blue blue value
     * 
     * @returns nothing.
     **/
    void actualizeValue(uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief Changing operating moge of RGB driver. It will be possible to operate one color in mnual and another color in auto.
     * 
     * @param redMode red mode
     * @param greenMode green mode
     * @param blueMode blue mode
     * @param sequenceNo number of selected sequence
     * @param period time between two steps in auto mode (for example change from red to green will take 2 seconds if period is equal to 2000) [ms]
     * 
     * @returns nothing.
     **/
    void actualizeMode(uint8_t redMode, uint8_t greenMode, uint8_t blueMode, uint8_t sequenceNo, uint32_t period);
    void actualizePeriod(uint16_t period);
    /**
     * @brief TODO!
     **/
    void createSequence(colors_t first, colors_t through, colors_t last, uint32_t period);

    /**
     * @brief Main function of the RGB driver, infinity loop!
     * @param pvParameters rgbController_t type, while calling function it must be forwarded configured 3 x PWM channels and one Timer. Main function is responsible for setting values to PWM pins.
     **/
    void rgbController_main(void *pvParameters);
    void rgbController_init();
}

class Color {
    private:
        uint8_t value;
        uint8_t mode;
    public:
    /* set mutexs - more than one thread can void this function.*/
        void setValue(uint8_t val) {
            std::lock_guard<std::mutex> lock(mtxVal);
            value = val; 
        }
        void setMode(uint8_t mod) {
            std::lock_guard<std::mutex> lock(mtxMod);
            mode = mod; 
        }
        uint8_t getValue(void) {
            std::lock_guard<std::mutex> lock(mtxVal);
            return value; 
        }
        uint8_t getMode(void) { 
            std::lock_guard<std::mutex> lock(mtxMod);
            return mode; 
        }
};

void sequenceMode();
