#include "ir_sensor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "gpio_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_intr_alloc.h"

static const char* TAG = "IrSensor";


#define ESP_INTR_FLAG_DEFAULT 0
#define IR_SENSOR_DEBOUNCE_MS 50
#define IR_SENSOR_COUNT 4

// This will be set to true once all things like queue, ISR install are done
static bool setupRan = false;

/* 
--------------------------------------------
------ISR and event related functions ------ 
--------------------------------------------
*/

// Static (file-scope) vars for the components internal state
// This is a queue from FreeRTOS that acts as the communcation channel
static QueueHandle_t sIrSensorEvtQueue = NULL;

// This stores the handle(indentifier) of the FreeRTOS task that processes events
static TaskHandle_t sIrSensorTaskHandle = NULL;

// Array that holds pointers to IR sensor data
static ir_sensor_data_t *sIrSensorsData[IR_SENSOR_COUNT];

ir_sensor_data_t* grab_ir_sensor_data(int pinNum)
{
    return sIrSensorsData[pinNum - 1];
}


// --- Internal ISR Handler
// This function runs in interrupt context. Keep it as short as possible.
// Its main job is to send the GPIO number to the queue.
// IRAM_ATTR tells the mcu to place this function into IRAM not Flash memory
static void IRAM_ATTR ir_sensor_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Send event to queue
    xQueueSendFromISR(sIrSensorEvtQueue, &gpio_num, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

// --- Internal Processing Task ---
// This task runs in task context and processes events from the queue.
// This is where your FSM logic or other complex processing for IR sensors would go.
/*
Things that need to be added/kept in mind:
- Infinite Loop (If it exits loop, it will "die")
- Blocking on queue (The task will be blocked most of the time and only
get unblocked when an queue event happens)
- Receving data (Reading the queue)
- Confirm pin state(Crucial for edge interrupts)
- Debouncing 
- FSM Logic 
- Trigger Actions
- Etc

This task will push state changes to the central queue
*/
static void ir_sensor_processing_task(void* arg)
{
    uint32_t pin_num;
    while (true)
    {
        if (xQueueReceive(sIrSensorEvtQueue, &pin_num, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            if (pin_num >= 48) {
                ESP_LOGE(TAG, "Invalid pin number");
                continue;
            }
            ir_sensor_data_t* sensorData = grab_ir_sensor_data(pin_num);
            bool currentLevel = get_gpio_value(pin_num);
            switch (sensorData->currentState)
            {
            case IR_STATE_INIT:
                if (currentLevel) {
                    sensorData->currentState = IR_STATE_STABLE_CLEAN;
                    ESP_LOGI(TAG, "Changed state from IR_STATE_INIT to IR_STATE_STABLE_CLEAN");
                } else {
                    sensorData->currentState = IR_STATE_STABLE_DETECTED;
                    ESP_LOGI(TAG, "Changed state from IR_STATE_INIT to IR_STATE_STABLE_DETECTED");
                }
                break;

            case IR_STATE_STABLE_DETECTED:
                if (currentLevel) {
                    sensorData->currentState = IR_STATE_DEBOUNCING_CLEAN;
                    sensorData->lastLevelChangeTime = xTaskGetTickCount();
                    ESP_LOGI(TAG, "Changed state from IR_STATE_STABLE_DETECTED to IR_STATE_DEBOUNCING_CLEAN");
                }
                // If 0 then stay in state
                break;
            
            case IR_STATE_STABLE_CLEAN:
                if (!currentLevel) {
                    sensorData->currentState = IR_STATE_DEBOUNCING_DETECTED;
                    sensorData->lastLevelChangeTime = xTaskGetTickCount();
                    ESP_LOGI(TAG, "Changed state from IR_STATE_STABLE_CLEAN to IR_STATE_DEBOUNCING_DETECTED");
                }
                break;

            default:
                break;
            }
            sensorData->previousLevel = currentLevel;
        }

        // Time Evaluation
        // Sweep through sensors to see if there are any waiting for delay to finish
        TickType_t currentTicks = xTaskGetTickCount();    

        for (int i = 0; i < IR_SENSOR_COUNT; i++)
        {
            ir_sensor_data_t* sensor = sIrSensorsData[i];
            if (sensor == NULL) continue;

            if (sensor->currentState == IR_STATE_DEBOUNCING_CLEAN ||
                sensor->currentState == IR_STATE_DEBOUNCING_DETECTED)
            {
                // Check if required time has passed since last edge
                if ((currentTicks - sensor->lastLevelChangeTime) >= pdMS_TO_TICKS(IR_SENSOR_DEBOUNCE_MS))
                {
                    // Time has passed and we can assume stable level
                    bool stableLevel = get_gpio_value(sensor->pinNum);

                    if (stableLevel) {
                        sensor->currentState = IR_STATE_STABLE_CLEAN;
                    } else {
                        sensor->currentState = IR_STATE_STABLE_DETECTED;
                    }
                }
            }
        }
    }
}

esp_err_t full_ir_init()
{
    sIrSensorEvtQueue = xQueueCreate(10, sizeof(uint32_t)); // Create the queue
    if (sIrSensorEvtQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create IR Sensor event queue");
        return ESP_FAIL;
    }

    // Create the task
    BaseType_t task_created = xTaskCreate(ir_sensor_processing_task,
                              "ir_sensor_processing task",
                               2048,     // Stack size in words
                               NULL,     // Parameter passed to task
                               10,       // Task priority
                               &sIrSensorTaskHandle); // Store task handle
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create IR Sensor processing task");
        vQueueDelete(sIrSensorEvtQueue); // Clean up
        return ESP_FAIL;
    } 

    // Install isr service
    esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { //ESP_ERR_INVALID_STATE means already installed
        ESP_LOGE(TAG, "Failed to install GPIO ISR Service Error in full_ir_init: %s", esp_err_to_name(ret));
        vQueueDelete(sIrSensorEvtQueue);
        vTaskDelete(sIrSensorTaskHandle);
        return ret;
    }   
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "GPIO ISR service installed.");
    } else if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "GPIO ISR service already installed.");
    }


    setupRan = true;
    return ESP_OK;
}


// --- Public Initialization Function ---
// This is the public api function to be called by main.c
// SHOULD ONLY BE CALLED ONCE PER RUNTIME
// This will set the GPIO pin to input and setup the sensorData var
esp_err_t ir_sensor_init(int pinNum, ir_sensor_data_t* sensorData)
{
    esp_err_t ret;
    if (!setupRan) {
        ret = full_ir_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error has occured with full_ir_init\n Error: %s\n", esp_err_to_name(ret));
            return ret;  // Return the error given
    }
    }
    // Check for null ptr
    if (sensorData == NULL)
    {
        ESP_LOGE(TAG, "sensorData pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ret = set_gpio_to_input(pinNum, false, false, GPIO_INTR_ANYEDGE);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error has occured with ir_sensor_init\n Error: %s\n", esp_err_to_name(ret));
        return ret;  // Return the error given
    }

    ret = gpio_isr_handler_add(pinNum, ir_sensor_isr_handler, (void *)pinNum);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ir_sensor_init error when adding handler to pin num: %d\n error: %s",
                pinNum,
                esp_err_to_name(ret));
        return ret;
    }

    sensorData->pinNum = pinNum;
    sensorData->currentState = IR_STATE_INIT;
    sensorData->lastLevelChangeTime = xTaskGetTickCount();
    sensorData->previousLevel = get_gpio_value(pinNum);
    sIrSensorsData[pinNum - 1] = sensorData;
    
    return ESP_OK;
}