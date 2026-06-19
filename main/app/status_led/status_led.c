#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "status_led.h"

#define STATUS_LED_GPIO         GPIO_NUM_14
#define TAG_BLINK_PERIOD_MS     200
#define LED_ON_LEVEL            0
#define LED_OFF_LEVEL           1

static volatile bool tag_detected;

static void status_led_task(void *arg)
{
    bool led_on = false;

    while (1) {
        if (tag_detected) {
            led_on = !led_on;
            gpio_set_level(STATUS_LED_GPIO, led_on ? LED_ON_LEVEL : LED_OFF_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(TAG_BLINK_PERIOD_MS));
        } else {
            led_on = false;
            gpio_set_level(STATUS_LED_GPIO, LED_OFF_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(TAG_BLINK_PERIOD_MS));
        }
    }
}

void status_led_init(void)
{
    gpio_config_t pin = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&pin);
    gpio_set_level(STATUS_LED_GPIO, LED_OFF_LEVEL);

    xTaskCreate(status_led_task,
                "status_led_task",
                2048,
                NULL,
                3,
                NULL);
}

void status_led_set_tag_detected(bool detected)
{
    tag_detected = detected;
}
