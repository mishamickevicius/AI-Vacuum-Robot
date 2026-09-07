#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "esp_err.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"

// State names enumerated 
typedef enum {
    IR_STATE_INIT = 0,
    IR_STATE_STABLE_DETECTED, // 1 
    IR_STATE_DEBOUNCING_CLEAN, // 2
    IR_STATE_STABLE_CLEAN,    // 3
    IR_STATE_DEBOUNCING_DETECTED // 4
} ir_sensor_state_t;

// Struct that holds FSM state and relavent data for ONE sensor
typedef struct {
    int pinNum;
    ir_sensor_state_t currentState;
    bool previousLevel;
    TickType_t lastLevelChangeTime;

} ir_sensor_data_t;

// Gets the pointer to ir sensor data based on pin
ir_sensor_data_t* grab_ir_sensor_data(int pin_num);
// Initalize the gpio pin and set up data var
esp_err_t ir_sensor_init(int pinNum, ir_sensor_data_t* sensorData);
// Initialize the event queue, install isr, update static vars
esp_err_t full_ir_init();

#endif