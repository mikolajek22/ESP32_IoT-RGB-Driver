#include "rgbController.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#define RED     0
#define GREEN   1
#define BLUE    2

#define DUTY_RESOLUTION     2^8
#define MAX_DUTY_VAL        2*DUTY_RESOLUTION

Color redColorHandler;
Color greenColorHandler;
Color blueColorHandler;

typedef struct color_t{
    uint8_t mode;
    uint8_t value;
};
color_t color[3];
uint8_t generalMode = false;
uint32_t periodTime;
uint8_t seqNo = 0;
uint16_t RGBTaskDelay = 100;

// original sequence stats
colors_t baseRgbValues[3] = {0};
uint16_t periodOriginal = 1;
bool resetOriginalFlag;

typedef struct {
    ledc_channel_config_t *ledc_channel[3];
    ledc_timer_config_t *ledc_timer;
} rgbController_t;

void actualizePeriod(uint16_t period) {
    RGBTaskDelay = period;
    ESP_LOGI("RGB", "RECEIVED PERIOD IS: %d", period);
}

void actualizeValue(uint8_t red, uint8_t green, uint8_t blue){
    redColorHandler.setValue(red);
    greenColorHandler.setValue(green);
    blueColorHandler.setValue(blue);
}

void actualizeMode(uint8_t redMode, uint8_t greenMode, uint8_t blueMode, uint8_t sequenceNo, uint32_t period){
    redColorHandler.setMode(redMode);
    greenColorHandler.setMode(greenMode);
    blueColorHandler.setMode(blueMode);
    periodTime = period;
    seqNo = sequenceNo;
    ESP_LOGI("rgb","mode actualized!");
}

void sequentialMode(uint8_t sNo, uint32_t period){
    static uint8_t tempRed;
    static uint8_t tempGreen;
    static uint8_t tempBlue;

    static bool    init = false;
    static uint8_t prevSNo;
    static uint8_t step;
    uint32_t startTick = 0;
    uint32_t sumTicks = 0;
    uint32_t timeDelay = period/255;

    if (prevSNo != sNo) {
        step = 1;
        init = false;
    }

    switch(sNo){

        /* RED -> GREEN -> BLUE -> RED  (fade) */
        case 1:
            if (!init) {
                init = true;
                //default red
                tempRed = 255;
                tempGreen = 0;
                tempBlue = 0;
                step = 1;
            }
            else {
                switch (step){
                    case 1:
                        //change to green
                        tempRed--;
                        tempGreen++;
                        if (tempRed == 0 && tempGreen == 255) { step++; }
                        break;
                    case 2:
                        //change to blue
                        tempGreen--;
                        tempBlue++;
                        if (tempGreen == 0 && tempBlue == 255) { step++; }
                        break;
                    case 3:
                        //change to red
                        tempBlue--;
                        tempRed++;
                        if (tempBlue == 0 && tempRed == 255) { step = 1; }
                        break;
                    default:
                        step = 1;
                        break;
                }
            }

            /* period between 2 phases */
            // startTick = xTaskGetTickCount();
            // do {
            //     sumTicks = xTaskGetTickCount() - startTick;
            // } while (10 * sumTicks < timeDelay);

            break;

        /* PINK -> SEA -> YELLOW -> PINK  (fade) */
        case 2:
            if (!init) {
                init = true;
                //default pink
                tempRed = 255;
                tempGreen = 0;
                tempBlue = 255;
                step = 1;
            }
            else {
                switch (step){
                    case 1:
                        //change to sea
                        tempRed--;
                        tempGreen++;
                        if (tempRed == 0 && tempGreen == 255) { step++; vTaskDelay(pdMS_TO_TICKS(100));}
                        break;
                    case 2:
                        //change to yellow
                        tempBlue--;
                        tempRed++;
                        if (tempRed == 255 && tempBlue == 0) { step++; vTaskDelay(pdMS_TO_TICKS(100)); }
                        break;
                    case 3:
                        //change to pink
                        tempBlue++;
                        tempGreen--;
                        if (tempBlue == 255 && tempGreen == 0) { step = 1; vTaskDelay(pdMS_TO_TICKS(100)); }
                        break;
                    default:
                        step = 1;
                        break;
                }
            }

            /* period between 2 phases */
            // startTick = xTaskGetTickCount();
            // do {
            //     sumTicks = xTaskGetTickCount() - startTick;
            // } while (10 * sumTicks < timeDelay);

            break;

        /* BLUE -> PINK -> RED -> PINK -> BLUE (fade)*/
        case 3:
            if (!init) {
                init = true;
                //default pink
                tempRed = 0;
                tempGreen = 0;
                tempBlue = 255;
                step = 1;
            }
            else {
                switch (step){
                    case 1:
                        //change to pink
                        tempRed++;
                        if (tempRed == 255) { step++; vTaskDelay(pdMS_TO_TICKS(100)); }
                        break;
                    case 2:
                        //change to red
                        tempBlue--;
                        if (tempBlue == 0) { step++; vTaskDelay(pdMS_TO_TICKS(100)); }
                        break;
                    case 3:
                        //change to pink
                        tempBlue++;
                        if (tempBlue == 255) { step++; vTaskDelay(pdMS_TO_TICKS(100)); }
                        break;
                    case 4:
                        //change to pink
                        tempRed--;
                        if (tempRed == 0) { step = 1; vTaskDelay(pdMS_TO_TICKS(100)); }
                        break;
                    default:
                        step = 1;
                        break;
                }
            }

            /* period between 2 phases */
            // startTick = xTaskGetTickCount();
            // do {
            //     sumTicks = xTaskGetTickCount() - startTick;
            // } while (10 * sumTicks < timeDelay);
            
            break;
        
        /* RED -> GREEN -> BLUE (fast change)*/
        case 4:
            if (!init) {
                init = true;
                //default RED
                tempRed = 255;
                tempGreen = 0;
                tempBlue = 0;
                step = 1;
            }
            else {
                switch (step){
                    case 1:
                        //change to green
                        tempRed = 0;
                        tempGreen = 255;
                        step++;
                        break;
                    case 2:
                        //change to blue
                        tempGreen = 0;
                        tempBlue = 255;
                        step++;
                        break;
                    case 3:
                        //change to red
                        tempBlue = 0;
                        tempRed = 255;
                        step = 1;
                        break;
                    default:
                        step = 1;
                        break;
                }
            }

            /* period between 2 phases */
            // startTick = xTaskGetTickCount();
            // do {
            //     sumTicks = xTaskGetTickCount() - startTick;
            // } while (10 * sumTicks < timeDelay);
            // break;
        break;
        default:
            ESP_LOGW("rgb","sequence number unknown????");
            break;
    }
    color[RED].value = tempRed;
    color[GREEN].value = tempGreen;
    color[BLUE].value = tempBlue;
    prevSNo = sNo;
}



float tempRed = 0;
float tempGreen = 0;
float tempBlue = 0;

void originalMode(colors_t first, colors_t through, colors_t last, uint32_t period) {
    
    static float diffRed;
    static float diffGreen;
    static float diffBlue;

    static uint8_t absBlueDiff;
    static uint8_t absGreenDiff;
    static uint8_t absRedDiff;
    static colors_t actualColor;
    static colors_t nextColor;

    static uint8_t step;  /* 1 - first -> second, 2- second -> third, 3 - third -> first*/
    if (resetOriginalFlag) {
        step = 1;
        resetOriginalFlag = false;
        actualColor = first;
        nextColor = through;

        absRedDiff = (uint8_t)abs(actualColor.red - nextColor.red);
        absGreenDiff = (uint8_t)abs(actualColor.green - nextColor.green);
        absBlueDiff = (uint8_t)abs(actualColor.blue - nextColor.blue);

        diffRed = (float)absRedDiff * periodOriginal/100;
        diffGreen = (float)absGreenDiff * periodOriginal/100;
        diffBlue = (float)absBlueDiff * periodOriginal/100;
    }
    else {
        /*  ALGHORITM DESCIPTION : 
        * In case of different target values of colors in base point the algorithm should calculate value of incrementation by each step. For example in case, where starting point is (0,0,0)
        * and the second base point is (256,128,64). In this example period is set to 2000ms. In that case values of red should be incremented in 2 seconds from 0 to 256, green from 0 to 128 and blue from 0 to 64.
        * It is necessery to find lowest value from next base point and calculate value of incrementation for each color.
        * 
        * Additionally, wait time should be calculated in order to the smallest value per each step.
        */
    //    ESP_LOGW("RGB","ACT and NEXT %d, %d, %d",actualColor.red, nextColor.red, actualColor.blue/*color[RED].value,color[GREEN].value,color[BLUE].value*/);
        
        // ESP_LOGW("RGB","diff is: %d, %d, %d",absRedDiff,absGreenDiff,absBlueDiff);
        // uint8_t lowestVal = (absRedDiff < absGreenDiff) ? (absRedDiff < absBlueDiff ? absRedDiff : absBlueDiff) : (absGreenDiff < absBlueDiff ? absGreenDiff : absBlueDiff);
        // if (lowestVal == 0) { lowestVal = 1;} 


        

        // ESP_LOGI("RGB","real diff is: %.2f, %.2f, %.2f",diffRed,diffGreen,diffBlue);
        // ESP_LOGI("RGB","ABS diff is: %d, %d, %d",absRedDiff,absGreenDiff,absBlueDiff);
        // ESP_LOGE("RGB","PERIOD IS %.2f", (float)periodOriginal);
        // ESP_LOGW("RGB","TEMP VALUES: %.2f, %.2f, %.2f",tempRed,tempGreen,tempBlue);
        bool isRedReady = false;
        bool isGreenReady = false;
        bool isBlueReady = false;

        // red:
        if (actualColor.red - nextColor.red > 0) {
            // --
            if (actualColor.red - diffRed >= nextColor.red) {
                actualColor.red -= diffRed;
            }
            else {
                actualColor.red = nextColor.red;
            }
            
        }
        else if (actualColor.red - nextColor.red < 0) {
            // ++
            if (actualColor.red + diffRed <= nextColor.red) {
                actualColor.red += diffRed;
            }
            else {
                actualColor.red = nextColor.red;
            }
        }
        else {
            isRedReady = true;
        }

        // green
        if (actualColor.green - nextColor.green > 0) {
            // --
            if (actualColor.green - diffGreen >= nextColor.green) {
                actualColor.green -= diffGreen;
            }
            else {
                actualColor.green = nextColor.green;
            }
        }
        else if (actualColor.green - nextColor.green < 0) {
            // ++
            if (actualColor.green + diffGreen <= nextColor.green) {
                actualColor.green += diffGreen;
            }
            else {
                actualColor.green = nextColor.green;
            }
        }
        else {
            isGreenReady = true;
        }

        // blue
        if (actualColor.blue - nextColor.blue > 0) {
            // --
            if (actualColor.blue - diffBlue >= nextColor.blue) {
                actualColor.blue -= diffBlue;
            }
            else {
                actualColor.blue = nextColor.blue;
            }
        }
        else if (actualColor.blue - nextColor.blue < 0) {
            // ++
            if (actualColor.blue + diffBlue <= nextColor.blue) {
                actualColor.blue += diffBlue;
            }
            else {
                actualColor.blue = nextColor.blue;
            }
        }
        else {
            isBlueReady = true;
        }

        if (isRedReady && isGreenReady && isBlueReady) { 
            if (step != 3){
                step++;
            }
            else {
                step = 1;
            }
            switch (step){
                case 1:
                    actualColor = first;
                    nextColor = through;
                    absRedDiff = (uint8_t)abs(actualColor.red - nextColor.red);
                    absGreenDiff = (uint8_t)abs(actualColor.green - nextColor.green);
                    absBlueDiff = (uint8_t)abs(actualColor.blue - nextColor.blue);

                    diffRed = (float)absRedDiff * periodOriginal/100;
                    diffGreen = (float)absGreenDiff * periodOriginal/100;
                    diffBlue = (float)absBlueDiff * periodOriginal/100;
                    break;
                case 2:
                    actualColor = through;
                    nextColor = last;
                    absRedDiff = (uint8_t)abs(actualColor.red - nextColor.red);
                    absGreenDiff = (uint8_t)abs(actualColor.green - nextColor.green);
                    absBlueDiff = (uint8_t)abs(actualColor.blue - nextColor.blue);

                    diffRed = (float)absRedDiff * periodOriginal/100;
                    diffGreen = (float)absGreenDiff * periodOriginal/100;
                    diffBlue = (float)absBlueDiff * periodOriginal/100;
                    break;
                case 3:
                    actualColor = last;
                    nextColor = first;
                    absRedDiff = (uint8_t)abs(actualColor.red - nextColor.red);
                    absGreenDiff = (uint8_t)abs(actualColor.green - nextColor.green);
                    absBlueDiff = (uint8_t)abs(actualColor.blue - nextColor.blue);

                    diffRed = (float)absRedDiff * periodOriginal/100;
                    diffGreen = (float)absGreenDiff * periodOriginal/100;
                    diffBlue = (float)absBlueDiff * periodOriginal/100;
                    break;
            } 
        }
        isRedReady = false;
        isGreenReady = false;
        isBlueReady = false;

    }

    color[RED].value = actualColor.red;
    color[GREEN].value = actualColor.green;
    color[BLUE].value = actualColor.blue;

    // uint32_t startTick = 0;
    // uint32_t sumTicks = 0;
    // uint32_t timeDelay = period/255;
    // startTick = xTaskGetTickCount();
    // do {
    //     sumTicks = xTaskGetTickCount() - startTick;
    // } while (10 * sumTicks < timeDelay);
}


void createSequence(colors_t first, colors_t through, colors_t last, uint32_t period){
    baseRgbValues[0] = first;
    baseRgbValues[1] = through;
    baseRgbValues[2] = last;
    tempRed = abs(baseRgbValues[0].red);
    tempGreen = abs(baseRgbValues[0].green);
    tempBlue = abs(baseRgbValues[0].blue);
    periodOriginal = (uint16_t)period;
    ESP_LOGI("RGB","PERIOD IS %d", periodOriginal);
    // ESP_LOGI("RGB","sequence created");
    // in order to reload sequence.
    resetOriginalFlag = true;
}

void rgbController_main(void *pvParameters) {
    static rgbController_t *controllerCfg = static_cast<rgbController_t*>(pvParameters);

    while (true) {
        color[RED].mode = redColorHandler.getMode();
        color[GREEN].mode = greenColorHandler.getMode();
        color[BLUE].mode = blueColorHandler.getMode();

        generalMode = color[BLUE].mode + color[GREEN].mode + color[RED].mode;
        if (generalMode == 0){           // Manual mode
            color[RED].value = redColorHandler.getValue();
            color[GREEN].value = greenColorHandler.getValue();
            color[BLUE].value = blueColorHandler.getValue();
            uint32_t startTick = xTaskGetTickCount();
            uint32_t sumTicks=0;
            /* wait for the moment */
            // do {
            //     sumTicks = xTaskGetTickCount() - startTick;
            // } while (10 * sumTicks < 100);
            // vTaskDelay(pdMS_TO_TICKS(50));
        }

        else if (generalMode == 3) {     // Sequence mode
            sequentialMode(seqNo,  periodTime); 
        }
        else if (generalMode == 6){
            // printf("mode fault");
            originalMode(baseRgbValues[0], baseRgbValues[1], baseRgbValues[2], periodOriginal);
            // ESP_LOGW("RGB","ORIGINAL");
        }

        /* change values of duty cycle */
        ledc_set_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[RED]->channel, color[RED].value);
        ledc_set_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[GREEN]->channel, color[GREEN].value);
        ledc_set_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[BLUE]->channel, color[BLUE].value);

        /* update values of duty cycle*/
        ledc_update_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[RED]->channel);
        ledc_update_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[GREEN]->channel);
        ledc_update_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[BLUE]->channel);
            // printf("twoja statra to kopara\n");
        // ESP_LOGW("RGB","first %d, %d, %d",baseRgbValues[0].red, baseRgbValues[0].green, baseRgbValues[0].blue/*color[RED].value,color[GREEN].value,color[BLUE].value*/);
        // vTaskDelay(pdMS_TO_TICKS(100));
        // ESP_LOGW("RGB","second %d, %d, %d",baseRgbValues[1].red, baseRgbValues[1].green, baseRgbValues[1].blue/*color[RED].value,color[GREEN].value,color[BLUE].value*/);
        // vTaskDelay(pdMS_TO_TICKS(100));
        // ESP_LOGW("RGB","third %d, %d, %d",baseRgbValues[2].red, baseRgbValues[2].green, baseRgbValues[2].blue/*color[RED].value,color[GREEN].value,color[BLUE].value*/);
        vTaskDelay(pdMS_TO_TICKS(RGBTaskDelay));
        // ESP_LOGW("RGB","written %d, %d, %d",color[RED].value,color[GREEN].value,color[BLUE].value);
        // vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}