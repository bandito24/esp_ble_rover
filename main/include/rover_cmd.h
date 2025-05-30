#pragma once
#include "stdint.h"

#define PWM_DUTY_RESOLUTION LEDC_TIMER_8_BIT
#define PWM_FREQUENCY 5000
#define PWM_SPEED_MODE LEDC_LOW_SPEED_MODE
#define PWM_TIMER_NUM LEDC_TIMER_0
#define PWM_CLK_CFG LEDC_AUTO_CLK

// LEDC Channel Configuration Macros
#define PWM_INITIAL_DUTY 0
#define PWM_HPOINT 0


typedef enum { // DO NOT mess with the ordering
  IDLE = 0,
  MOVE_STRAIGHT = 1,
  MOVE_BACKWARD = 2,
  MOVE_LEFT = 3,
  MOVE_RIGHT = 4,
  ACTIVATE_TURBO = 5,
  DEACTIVATE_TURBO = 6
} rover_command;

esp_err_t rover_wheels_init();
void rover_command_task(void *param);
void interpret_rover_command(rover_command command);