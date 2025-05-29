#pragma once
#include "common.h"



esp_err_t led_init();
void led_conn_task(void *param);
void led_set_is_connected(bool conn);
