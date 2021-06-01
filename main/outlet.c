#include "outlet.h"

#include <driver/gpio.h>
#include <freertos/task.h>

#define FLIP_FLOP_CLK_GPIO 4
#define FLIP_FLOP_D_GPIO 5

#define GPIO_BITMASK ((1ULL << FLIP_FLOP_CLK_GPIO) | (1ULL << FLIP_FLOP_D_GPIO))

void init_outlet_gpio() {
	gpio_config_t gpio_conf = { 0 };
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = GPIO_BITMASK;
	gpio_config(&gpio_conf);
	gpio_set_level(FLIP_FLOP_CLK_GPIO, 0);
	gpio_set_level(FLIP_FLOP_D_GPIO, 0);
}

void set_outlet(bool val) {
	// We set the flip-flop by setting the data pin and cycling the clock once
	gpio_set_level(FLIP_FLOP_D_GPIO, val);
	gpio_set_level(FLIP_FLOP_CLK_GPIO, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(FLIP_FLOP_CLK_GPIO, 0);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	gpio_set_level(FLIP_FLOP_D_GPIO, 0);
}