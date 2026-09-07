#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==========================================
// Hardware Pin Definitions (Vacuum Robot)
// ==========================================
#define SUCTION_MOTOR_PWM_PIN       GPIO_NUM_16
#define SUCTION_MOTOR_EN_PIN        GPIO_NUM_17
#define BRUSH_MOTOR_PWM_PIN         GPIO_NUM_18
#define BRUSH_MOTOR_EN_PIN          GPIO_NUM_19

// ==========================================
// Suction Motor API
// ==========================================
esp_err_t suction_motor_enable(void);
esp_err_t suction_motor_stop(void);
esp_err_t suction_motor_set_speed(uint32_t speed_duty);

// ==========================================
// Brush Motor API
// ==========================================
esp_err_t brush_motor_enable(void);
esp_err_t brush_motor_stop(void);
esp_err_t brush_motor_set_speed(uint32_t speed_duty);

// ==========================================
// General Initialization
// ==========================================
esp_err_t motor_control_init(void);
esp_err_t init_pwm_module(void);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_CONTROL_H
