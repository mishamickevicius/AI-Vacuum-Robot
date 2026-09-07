#ifndef ROBOT_ORCHESTRATOR_H
#define ROBOT_ORCHESTRATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==========================================
// Robot System States
// ==========================================
typedef enum {
    ROBOT_STATE_IDLE = 0,
    ROBOT_STATE_CLEANING_MODE,      // Replaces MOWING_MODE
    ROBOT_STATE_OBSTACLE_AVOIDANCE,
    ROBOT_STATE_DOCKING,
    ROBOT_STATE_CHARGING,
    ROBOT_STATE_ERROR
} robot_state_t;

// Backward-compatible alias for explicit CLEANING_MODE references
#define CLEANING_MODE ROBOT_STATE_CLEANING_MODE

// ==========================================
// Orchestrator Event Types
// ==========================================
typedef enum {
    ORCHESTRATOR_EVENT_START_CLEANING,
    ORCHESTRATOR_EVENT_STOP_CLEANING,
    ORCHESTRATOR_EVENT_OBSTACLE_DETECTED,
    ORCHESTRATOR_EVENT_OBSTACLE_CLEARED,
    ORCHESTRATOR_EVENT_LOW_BATTERY,
    ORCHESTRATOR_EVENT_DOCK_REACHED,
    ORCHESTRATOR_EVENT_ERROR
} orchestrator_event_type_t;

typedef struct {
    orchestrator_event_type_t type;
    int32_t data;
} orchestrator_event_t;

// ==========================================
// Orchestrator API
// ==========================================
esp_err_t robot_orchestrator_init(void);
esp_err_t robot_orchestrator_start(void);
esp_err_t robot_orchestrator_send_event(const orchestrator_event_t *event);
robot_state_t robot_orchestrator_get_state(void);
void robot_orchestrator_set_state(robot_state_t new_state);

#ifdef __cplusplus
}
#endif

#endif // ROBOT_ORCHESTRATOR_H
