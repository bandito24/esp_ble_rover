#include "led.h"


static void led_indicate_adv();
static void led_indicate_conn();
static volatile uint8_t conn_established = 0;


esp_err_t led_init(){
    gpio_config_t led_cfg = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT
    };
    return gpio_config(&led_cfg);
}

static void led_indicate_adv(){
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(300));
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(300));
}
static void led_indicate_conn(){
    gpio_set_level(LED_PIN, 1);
}
void led_set_is_connected(bool conn){
    conn_established = conn;
}

void led_conn_task(void *param){
    while(1){
        if(conn_established){
            led_indicate_conn();
        } else {
            led_indicate_adv();
        }

    vTaskDelay(pdMS_TO_TICKS(200));
    }
}