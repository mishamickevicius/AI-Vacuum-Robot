#include "motor_control.h"
#include "esp_log.h"
#include "gpio_driver.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"

static const char *TAG = "MotorControl";

#define PWM_FREQ 1000000
#define PERIOD_TICKS_COUNT 20000

// Static handles for Suction Motor MCPWM
static mcpwm_timer_handle_t timer = NULL;
static mcpwm_oper_handle_t suction_oper = NULL;
static mcpwm_cmpr_handle_t suction_cmpr = NULL;
static mcpwm_gen_handle_t suction_gen = NULL;

// Static handles for Brush Motor MCPWM
static mcpwm_oper_handle_t brush_oper = NULL;
static mcpwm_cmpr_handle_t brush_cmpr = NULL;
static mcpwm_gen_handle_t brush_gen = NULL;

static bool s_initialized = false;

esp_err_t init_pwm_module(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing vacuum motor MCPWM module...");

    // 1. Configure shared MCPWM timer
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = PWM_FREQ,
        .period_ticks = PERIOD_TICKS_COUNT,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    esp_err_t ret = mcpwm_new_timer(&timer_config, &timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create MCPWM timer: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. Setup Suction Motor Operator, Comparator, and Generator
    mcpwm_operator_config_t oper_config = { .group_id = 0 };
    ret = mcpwm_new_operator(&oper_config, &suction_oper);
    if (ret != ESP_OK) return ret;
    ret = mcpwm_operator_connect_timer(suction_oper, timer);
    if (ret != ESP_OK) return ret;

    mcpwm_comparator_config_t cmpr_config = {
        .flags.update_cmp_on_tez = true
    };
    ret = mcpwm_new_comparator(suction_oper, &cmpr_config, &suction_cmpr);
    if (ret != ESP_OK) return ret;

    mcpwm_generator_config_t suction_gen_config = {
        .gen_gpio_num = SUCTION_MOTOR_PWM_PIN,
    };
    ret = mcpwm_new_generator(suction_oper, &suction_gen_config, &suction_gen);
    if (ret != ESP_OK) return ret;

    // 3. Setup Brush Motor Operator, Comparator, and Generator
    ret = mcpwm_new_operator(&oper_config, &brush_oper);
    if (ret != ESP_OK) return ret;
    ret = mcpwm_operator_connect_timer(brush_oper, timer);
    if (ret != ESP_OK) return ret;

    ret = mcpwm_new_comparator(brush_oper, &cmpr_config, &brush_cmpr);
    if (ret != ESP_OK) return ret;

    mcpwm_generator_config_t brush_gen_config = {
        .gen_gpio_num = BRUSH_MOTOR_PWM_PIN,
    };
    ret = mcpwm_new_generator(brush_oper, &brush_gen_config, &brush_gen);
    if (ret != ESP_OK) return ret;

    // 4. Configure Generator Actions on Timer Event & Compare Event
    mcpwm_gen_timer_event_action_t gen_timer_action = {
        .direction = MCPWM_TIMER_DIRECTION_UP,
        .event = MCPWM_TIMER_EVENT_EMPTY,
        .action = MCPWM_GEN_ACTION_HIGH,
    };
    mcpwm_gen_compare_event_action_t gen_cmpr_action = {
        .direction = MCPWM_TIMER_DIRECTION_UP,
        .comparator = suction_cmpr,
        .action = MCPWM_GEN_ACTION_LOW,
    };

    mcpwm_generator_set_action_on_timer_event(suction_gen, gen_timer_action);
    mcpwm_generator_set_action_on_compare_event(suction_gen, gen_cmpr_action);

    gen_cmpr_action.comparator = brush_cmpr;
    mcpwm_generator_set_action_on_timer_event(brush_gen, gen_timer_action);
    mcpwm_generator_set_action_on_compare_event(brush_gen, gen_cmpr_action);

    // Initial compare values (0 duty)
    mcpwm_comparator_set_compare_value(suction_cmpr, 0);
    mcpwm_comparator_set_compare_value(brush_cmpr, 0);

    // 5. Enable and start timer
    mcpwm_timer_enable(timer);
    mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);

    // 6. Configure Enable Pins as outputs
    gpio_config_t en_conf = {
        .pin_bit_mask = (1ULL << SUCTION_MOTOR_EN_PIN) | (1ULL << BRUSH_MOTOR_EN_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&en_conf);
    gpio_set_level(SUCTION_MOTOR_EN_PIN, 0);
    gpio_set_level(BRUSH_MOTOR_EN_PIN, 0);

    s_initialized = true;
    ESP_LOGI(TAG, "Vacuum motor MCPWM initialized successfully.");
    return ESP_OK;
}

esp_err_t motor_control_init(void)
{
    return init_pwm_module();
}

esp_err_t suction_motor_enable(void)
{
    if (!s_initialized) {
        esp_err_t err = init_pwm_module();
        if (err != ESP_OK) return err;
    }
    ESP_LOGI(TAG, "Enabling suction motor");
    return gpio_set_level(SUCTION_MOTOR_EN_PIN, 1);
}

esp_err_t suction_motor_stop(void)
{
    ESP_LOGI(TAG, "Stopping suction motor");
    gpio_set_level(SUCTION_MOTOR_EN_PIN, 0);
    if (suction_cmpr) {
        mcpwm_comparator_set_compare_value(suction_cmpr, 0);
    }
    return ESP_OK;
}

esp_err_t suction_motor_set_speed(uint32_t speed_duty)
{
    if (speed_duty > PERIOD_TICKS_COUNT) {
        speed_duty = PERIOD_TICKS_COUNT;
    }
    if (!s_initialized) {
        esp_err_t err = init_pwm_module();
        if (err != ESP_OK) return err;
    }
    ESP_LOGI(TAG, "Setting suction motor speed: %lu ticks", (unsigned long)speed_duty);
    return mcpwm_comparator_set_compare_value(suction_cmpr, speed_duty);
}

esp_err_t brush_motor_enable(void)
{
    if (!s_initialized) {
        esp_err_t err = init_pwm_module();
        if (err != ESP_OK) return err;
    }
    ESP_LOGI(TAG, "Enabling brush motor");
    return gpio_set_level(BRUSH_MOTOR_EN_PIN, 1);
}

esp_err_t brush_motor_stop(void)
{
    ESP_LOGI(TAG, "Stopping brush motor");
    gpio_set_level(BRUSH_MOTOR_EN_PIN, 0);
    if (brush_cmpr) {
        mcpwm_comparator_set_compare_value(brush_cmpr, 0);
    }
    return ESP_OK;
}

esp_err_t brush_motor_set_speed(uint32_t speed_duty)
{
    if (speed_duty > PERIOD_TICKS_COUNT) {
        speed_duty = PERIOD_TICKS_COUNT;
    }
    if (!s_initialized) {
        esp_err_t err = init_pwm_module();
        if (err != ESP_OK) return err;
    }
    ESP_LOGI(TAG, "Setting brush motor speed: %lu ticks", (unsigned long)speed_duty);
    return mcpwm_comparator_set_compare_value(brush_cmpr, speed_duty);
}
