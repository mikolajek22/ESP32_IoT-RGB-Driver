#include <cstdlib>
#include <iostream>
#include <thread>


#define MANUAL_MODE 0
#define SEQUENCE_MODE 1
std::mutex mtxVal;
std::mutex mtxMod;
extern "C"{
    /* this functions are called from http_server.c! */
    void actualizeValue(uint8_t red, uint8_t green, uint8_t blue);
    void actualizeMode(uint8_t redMode, uint8_t greenMode, uint8_t blueMode);
}

class Color {
    private:
        uint8_t value;
        uint8_t mode;
    public:
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
void rgbController_main();