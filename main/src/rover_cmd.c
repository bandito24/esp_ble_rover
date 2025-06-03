#include "rover_cmd.h"
#include "common.h"
#include "driver/ledc.h"

#define BASE_DUTY 220

typedef enum {
  FRONT_LEFT,
  FRONT_RIGHT,
  BACK_LEFT,
  BACK_RIGHT,
  WHEEL_COUNT
} wheel_indication;
typedef enum { FORWARD = 1, BACKWARD = 0 } wheel_spin;

const char *WHEEL_TAG = "ROVER RESPONSE";
volatile static uint8_t power_multiplier = 1;
volatile static rover_command rover_status = IDLE;
volatile static bool updated_command = false;

static void move_left();
static void move_right();
static void move_backward();
static void move_forward();
static void dont_move();
static void stop_wheel(wheel_indication selected_wheel);
static void move_wheel(wheel_indication selected_wheel, wheel_spin direction);
static void activate_turbo(bool activate);
static void command_rover_wheels(rover_command command);

typedef struct {
  gpio_num_t in1_pin;
  gpio_num_t in2_pin;
  gpio_num_t enable_pin;
} rover_cmd_wheel;

const rover_cmd_wheel rover_wheels[4] = {
    [FRONT_LEFT] = {.in1_pin = GPIO_NUM_14,
                    .in2_pin = GPIO_NUM_15,
                    .enable_pin = GPIO_NUM_13},
    [FRONT_RIGHT] = {.in1_pin = GPIO_NUM_5,
                     .in2_pin = GPIO_NUM_27,
                     .enable_pin = GPIO_NUM_12},
    [BACK_LEFT] = {.in1_pin = GPIO_NUM_18,
                   .in2_pin = GPIO_NUM_19,
                   .enable_pin = GPIO_NUM_21},
    [BACK_RIGHT] = {.in1_pin = GPIO_NUM_22,
                    .in2_pin = GPIO_NUM_23,
                    .enable_pin = GPIO_NUM_25},
};
const ledc_channel_t pwm_channels[4] = {[FRONT_LEFT] = LEDC_CHANNEL_0,
                                        [FRONT_RIGHT] = LEDC_CHANNEL_1,
                                        [BACK_LEFT] = LEDC_CHANNEL_2,
                                        [BACK_RIGHT] = LEDC_CHANNEL_3};

void rover_command_task(void *param) {
  while (1) {
    command_rover_wheels(rover_status);
    vTaskDelay(1);
  }
}

esp_err_t rover_wheels_init() {
  esp_err_t res;
  esp_err_t rc = ESP_OK;
  for (int i = 0; i < 4; i++) {
    gpio_config_t wheel_cfg = {.pin_bit_mask =
                                   (1ULL << rover_wheels[i].in1_pin) |
                                   (1ULL << rover_wheels[i].in2_pin),
                               .mode = GPIO_MODE_DEF_OUTPUT};
    res = gpio_config(&wheel_cfg);
    if (res != ESP_OK) {
      rc = res;
      ESP_LOGE(WHEEL_TAG, "failure GPIO in1/in2 setup on wheel number %d", i);
    }

    ledc_timer_config_t ledc_timer = {.duty_resolution = PWM_DUTY_RESOLUTION,
                                      .freq_hz = PWM_FREQUENCY,
                                      .speed_mode = PWM_SPEED_MODE,
                                      .timer_num = PWM_TIMER_NUM,
                                      .clk_cfg = PWM_CLK_CFG};
    res = ledc_timer_config(&ledc_timer);
    if (res != ESP_OK) {
      rc = res;
      ESP_LOGE(WHEEL_TAG, "Failure PWM timer config for wheel number %d", i);
    } else {
      rc = res;
      ESP_LOGI(WHEEL_TAG, "PWM timer config for wheel number %d successful", i);
    }

    ledc_channel_config_t channel = {.channel = pwm_channels[i],
                                     .duty = PWM_INITIAL_DUTY,
                                     .gpio_num = rover_wheels[i].enable_pin,
                                     .speed_mode = PWM_SPEED_MODE,
                                     .hpoint = PWM_HPOINT,
                                     .timer_sel = PWM_TIMER_NUM};
    res = ledc_channel_config(&channel);
    if (res != ESP_OK) {
      rc = res;
      ESP_LOGE(WHEEL_TAG, "Failure PWM channel config for wheel number %d", i);
    } else {
      rc = res;
      ESP_LOGI(WHEEL_TAG, "PWM channel config for wheel number %d successful",
               i);
    }
  }
  return rc;
}

// this is what we will expose as the callback for ble
void interpret_rover_command(rover_command command) {
  if (command == ACTIVATE_TURBO) {
    activate_turbo(true);
    ESP_LOGI(WHEEL_TAG, "Activating turbo");
  } else if (command == DEACTIVATE_TURBO) {
    activate_turbo(false);
    ESP_LOGI(WHEEL_TAG, "Deactivating turbo");
  } else {
    if (rover_status != command) {
    updated_command = true;
    }
    rover_status = command;
  }
}

static void command_rover_wheels(rover_command command) {
  if(updated_command){
    dont_move();
    vTaskDelay(pdMS_TO_TICKS(1000));
    updated_command = false;
  }
  switch (command) {
  case IDLE:
    dont_move();
    break;
  case MOVE_STRAIGHT:
    move_forward();
    break;
  case MOVE_BACKWARD:
    move_backward();
    break;
  case MOVE_LEFT:
    move_left();
    break;
  case MOVE_RIGHT:
    move_right();
    break;
  default:
    rover_status = IDLE;
    ESP_LOGE(TAG, "Invalid command provided for rover");
  }
}

static void activate_turbo(bool activate) { power_multiplier = activate; }

static void move_wheel(wheel_indication selected_wheel, wheel_spin direction) {
  rover_cmd_wheel wheel = rover_wheels[selected_wheel];

  ESP_ERROR_CHECK(gpio_set_level(wheel.in1_pin, direction ? 1 : 0));
  ESP_ERROR_CHECK(gpio_set_level(wheel.in2_pin, direction ? 0 : 1));


  uint32_t raw_duty = BASE_DUTY * power_multiplier;
  uint32_t updated_duty = (raw_duty > 255) ? 255 : raw_duty;
  uint32_t curr_duty = ledc_get_duty(PWM_SPEED_MODE, pwm_channels[selected_wheel]);
  if (curr_duty < updated_duty) {
    updated_duty += 5;
  }

  ESP_ERROR_CHECK(
      ledc_set_duty(PWM_SPEED_MODE, pwm_channels[selected_wheel], updated_duty));
  ESP_ERROR_CHECK(
      ledc_update_duty(PWM_SPEED_MODE, pwm_channels[selected_wheel]));
}

static void move_left() {
  ESP_LOGI(WHEEL_TAG, "Moving left");
  move_wheel(FRONT_LEFT, BACKWARD);
  stop_wheel(FRONT_RIGHT);
  move_wheel(BACK_RIGHT, FORWARD);
  stop_wheel(BACK_LEFT);
}

static void move_right() {
  ESP_LOGI(WHEEL_TAG, "Moving right");
  move_wheel(FRONT_RIGHT, BACKWARD);
  stop_wheel(FRONT_LEFT);
  move_wheel(BACK_LEFT, FORWARD);
  stop_wheel(BACK_RIGHT);
}

static void move_forward() {

  ESP_LOGI(WHEEL_TAG, "Moving forward");
  for (int i = 0; i < WHEEL_COUNT; i++) {
    move_wheel(i, FORWARD);
  }
}

static void move_backward() {
  ESP_LOGI(WHEEL_TAG, "Moving backward");
  for (int i = 0; i < WHEEL_COUNT; i++) {
    move_wheel(i, BACKWARD);
  }
}

static void dont_move() {
  ESP_LOGI(WHEEL_TAG, "Rover is idle");
  for (int i = 0; i < WHEEL_COUNT; i++) {
    stop_wheel(i);
  }
}

static void stop_wheel(wheel_indication selected_wheel) {
  ESP_ERROR_CHECK(
      ledc_set_duty(PWM_SPEED_MODE, pwm_channels[selected_wheel], 0));
  ESP_ERROR_CHECK(
      ledc_update_duty(PWM_SPEED_MODE, pwm_channels[selected_wheel]));
}
