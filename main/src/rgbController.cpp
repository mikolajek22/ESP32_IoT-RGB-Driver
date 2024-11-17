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

typedef struct {
    ledc_channel_config_t *ledc_channel[3];
    ledc_timer_config_t *ledc_timer;
} rgbController_t;

void actualizeValue(uint8_t red, uint8_t green, uint8_t blue){
    redColorHandler.setValue(red);
    greenColorHandler.setValue(green);
    blueColorHandler.setValue(blue);
}

void sequentialMode(){

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
        }

        else if (generalMode == 3) {     // Sequence mode
            sequentialMode(); 
        }
        else {
            printf("mode fault");
        }

        /* change values of duty cycle */
        ledc_set_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[RED]->channel, 512);
        ledc_set_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[GREEN]->channel, 512);
        ledc_set_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[BLUE]->channel, 512);
// 2*color[RED].value
        /* update values of duty cycle*/
        ledc_update_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[RED]->channel);
        ledc_update_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[GREEN]->channel);
        ledc_update_duty(controllerCfg->ledc_timer->speed_mode, controllerCfg->ledc_channel[BLUE]->channel);
        // printf("twoja statra to kopara\n");
        ESP_LOGW("RGB","written %d, %d, %d",color[RED].value,color[GREEN].value,color[BLUE].value);
        vTaskDelay(500);
    }
    
}