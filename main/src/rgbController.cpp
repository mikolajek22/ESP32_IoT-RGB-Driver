#include "rgbController.h"

#define RED     0
#define GREEN   1
#define BLUE    2


Color redColorHandler;
Color greenColorHandler;
Color blueColorHandler;
typedef struct color_t{
    uint8_t mode;
    uint8_t value;
};
color_t color[3];
uint8_t generalMode = false;

void actualizeValue(uint8_t red, uint8_t green, uint8_t blue){
    redColorHandler.setValue(red);
    greenColorHandler.setValue(green);
    blueColorHandler.setValue(blue);
}

void manualMode(){

    
}
void sequentialMode(){

}

void rgbController_main() {
    while (true){
        color[RED].value = redColorHandler.getValue();
        color[GREEN].value = greenColorHandler.getValue();
        color[BLUE].value = blueColorHandler.getValue();

        color[RED].mode = redColorHandler.getMode();
        color[GREEN].mode = greenColorHandler.getMode();
        color[BLUE].mode = blueColorHandler.getMode();

        generalMode = color[BLUE].mode + color[GREEN].mode + color[RED].mode;
        if (generalMode == 0){           // Manual mode
            manualMode();
        }

        else if (generalMode == 3) {     // Sequence mode
            sequentialMode(); 
        }
        else {

        }
    }
    
}