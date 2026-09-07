/* 
Full Orchestrator FSM for Autonomous AI Vacuum Robot
Connects all the components in the system.
Implemented using a FreeRTOS Task in the main component.
Communicates using a system-wide event queue handled by the orchestrator.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "robot_orchestrator.h"
#include "motor_control.h"
#include "ir_sensor.h"

static const char *TAG = "RobotOrchestrator";

#define ORCHESTRATOR_QUEUE_LENGTH 16

static QueueHandle_t s_orchestrator_queue = NULL;
static robot_state_t s_current_state = ROBOT_STATE_IDLE;
static TaskHandle_t s_orchestrator_task_handle = NULL;

static const char *state_to_str(robot_state_t state)
{
    switch (state) {
        case ROBOT_STATE_IDLE:               return "IDLE";
        case ROBOT_STATE_CLEANING_MODE:      return "CLEANING_MODE";
        case ROBOT_STATE_OBSTACLE_AVOIDANCE: return "OBSTACLE_AVOIDANCE";
        case ROBOT_STATE_DOCKING:            return "DOCKING";
        case ROBOT_STATE_CHARGING:           return "CHARGING";
        case ROBOT_STATE_ERROR:              return "ERROR";
        default:                             return "UNKNOWN";
    }
}

void robot_orchestrator_set_state(robot_state_t new_state)
{
    if (s_current_state != new_state) {
        ESP_LOGI(TAG, "State transition: %s -> %s", state_to_str(s_current_state), state_to_str(new_state));
        s_current_state = new_state;

        switch (new_state) {
            case ROBOT_STATE_CLEANING_MODE:
                ESP_LOGI(TAG, "Entering CLEANING_MODE: powering up suction and brush motors");
                suction_motor_enable();
                suction_motor_set_speed(12000); // Nominal suction power
                brush_motor_enable();
                brush_motor_set_speed(10000);
                break;

            case ROBOT_STATE_OBSTACLE_AVOIDANCE:
                ESP_LOGI(TAG, "Entering OBSTACLE_AVOIDANCE: temporarily throttling suction");
                suction_motor_set_speed(5000);
                break;

            case ROBOT_STATE_DOCKING:
            case ROBOT_STATE_CHARGING:
            case ROBOT_STATE_IDLE:
            case ROBOT_STATE_ERROR:
                ESP_LOGI(TAG, "Cleaning inactive: shutting down suction and brush motors");
                suction_motor_stop();
                brush_motor_stop();
                break;

            default:
                break;
        }
    }
}

robot_state_t robot_orchestrator_get_state(void)
{
    return s_current_state;
}

esp_err_t robot_orchestrator_send_event(const orchestrator_event_t *event)
{
    if (!s_orchestrator_queue || !event) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(s_orchestrator_queue, event, (TickType_t)10) != pdPASS) {
        ESP_LOGW(TAG, "Orchestrator queue is full, dropping event %d", event->type);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void robot_orchestrator_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Vacuum robot orchestrator task started");
    orchestrator_event_t event;

    while (1) {
        if (xQueueReceive(s_orchestrator_queue, &event, portMAX_DELAY) == pdPASS) {
            ESP_LOGD(TAG, "Received orchestrator event: %d", event.type);

            switch (event.type) {
                case ORCHESTRATOR_EVENT_START_CLEANING:
                    robot_orchestrator_set_state(ROBOT_STATE_CLEANING_MODE);
                    break;

                case ORCHESTRATOR_EVENT_STOP_CLEANING:
                    robot_orchestrator_set_state(ROBOT_STATE_IDLE);
                    break;

                case ORCHESTRATOR_EVENT_OBSTACLE_DETECTED:
                    if (s_current_state == ROBOT_STATE_CLEANING_MODE) {
                        robot_orchestrator_set_state(ROBOT_STATE_OBSTACLE_AVOIDANCE);
                    }
                    break;

                case ORCHESTRATOR_EVENT_OBSTACLE_CLEARED:
                    if (s_current_state == ROBOT_STATE_OBSTACLE_AVOIDANCE) {
                        robot_orchestrator_set_state(ROBOT_STATE_CLEANING_MODE);
                    }
                    break;

                case ORCHESTRATOR_EVENT_LOW_BATTERY:
                    robot_orchestrator_set_state(ROBOT_STATE_DOCKING);
                    break;

                case ORCHESTRATOR_EVENT_DOCK_REACHED:
                    robot_orchestrator_set_state(ROBOT_STATE_CHARGING);
                    break;

                case ORCHESTRATOR_EVENT_ERROR:
                default:
                    robot_orchestrator_set_state(ROBOT_STATE_ERROR);
                    break;
            }
        }
    }
}

esp_err_t robot_orchestrator_init(void)
{
    if (s_orchestrator_queue != NULL) {
        return ESP_OK;
    }

    s_orchestrator_queue = xQueueCreate(ORCHESTRATOR_QUEUE_LENGTH, sizeof(orchestrator_event_t));
    if (s_orchestrator_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create orchestrator event queue");
        return ESP_ERR_NO_MEM;
    }

    s_current_state = ROBOT_STATE_IDLE;
    return ESP_OK;
}

esp_err_t robot_orchestrator_start(void)
{
    esp_err_t ret = robot_orchestrator_init();
    if (ret != ESP_OK) {
        return ret;
    }

    BaseType_t res = xTaskCreate(
        robot_orchestrator_task,
        "robot_orchestrator",
        4096,
        NULL,
        4,
        &s_orchestrator_task_handle
    );

    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}