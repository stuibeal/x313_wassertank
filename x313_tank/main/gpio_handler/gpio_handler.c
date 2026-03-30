/*
 * gpio_handler.c
 *
 *  Created on: 17.03.2026
 *      Author: al
 * @brief Functions for GPIO input and output
 * - creates a ISR for GPIO 4
 * - writes Data in xQueueRecieve
 * - starts gpio_task that handles xQueueRecieve
 */

#include "gpio_handler.h"
#include "driver/gpio.h"
#include <stdio.h>
// #include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
// #include <inttypes.h>
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

QueueHandle_t gpio_evt_queue = NULL;
static bool switchLedStatus;
static welcher_Knopf_t knopfGedrueckt;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
	uint32_t gpio_num = (uint32_t)arg;
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task(void *arg) {
	uint32_t io_num;
	for (;;) {
		if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			vTaskDelay(50); //debounce und so
			if (gpio_get_level(io_num)) {
				ESP_LOGI(GPIO_TAG,
						 "Knopf gedrückt: [%" PRIu32 "] intr, level: %d",
						 io_num, gpio_get_level(io_num));
				switch (io_num) {
				case GPIO_INPUT_SWITCH:
					knopfGedrueckt = links;
					break;
				case GPIO_INPUT_SWITCH_2:
					knopfGedrueckt = rechts;
					break;
				default:
					knopfGedrueckt = nicht;
					break;
				}
			}
		}
	}
}

void setup_gpio_pins(void) {
	gpio_config_t io_conf = {}; // Zero
	// INPUT GPIO SWITCH
	io_conf.pin_bit_mask = (1ULL << GPIO_INPUT_SWITCH); // Select GPIO 4
	io_conf.mode = GPIO_MODE_INPUT;						// Set as output
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;			// Enable pull-up
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;		// Disable pull-down
	io_conf.intr_type = GPIO_INTR_POSEDGE;				// Interrupt
	gpio_config(&io_conf);

	io_conf.pin_bit_mask = (1ULL << GPIO_INPUT_SWITCH_2); // Select GPIO 5
	io_conf.mode = GPIO_MODE_INPUT;						  // Set as output
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;			  // Enable pull-up
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;		  // Disable pull-down
	io_conf.intr_type = GPIO_INTR_POSEDGE;				  // Interrupt
	gpio_config(&io_conf);

	// OUTPUT GPIO PUMPE
	io_conf.pin_bit_mask = (1ULL << GPIO_OUTPUT_PUMPE); // Select GPIO 16
	io_conf.mode = GPIO_MODE_OUTPUT;					// Set as output
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;			// Disable pull-up
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;		// Disable pull-down
	io_conf.intr_type = GPIO_INTR_DISABLE; // Interrupt disable (logisch)
	gpio_config(&io_conf);

	// OUTPUT GPIO VENTIL
	io_conf.pin_bit_mask = (1ULL << GPIO_OUTPUT_VENTIL); // Select GPIO 17
	io_conf.mode = GPIO_MODE_OUTPUT;					 // Set as output
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;			 // Disable pull-up
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;		 // Disable pull-down
	io_conf.intr_type = GPIO_INTR_DISABLE; // Interrupt disable (logisch)
	gpio_config(&io_conf);

	// OUTPUT GPIO LED
	io_conf.pin_bit_mask = (1ULL << GPIO_OUTPUT_SWITCH_LED); // Select GPIO 15
	io_conf.mode = GPIO_MODE_OUTPUT;						 // Set as output
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;				 // Disable pull-up
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	io_conf.intr_type = GPIO_INTR_DISABLE;		  // Interrupt disable (logisch)
	gpio_config(&io_conf);

	// Power up Switch Led
	gpio_set_level(GPIO_OUTPUT_SWITCH_LED, 0);

	// start tasks
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	// start gpio task
	xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

	// install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	// hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_INPUT_SWITCH, gpio_isr_handler,
						 (void *)GPIO_INPUT_SWITCH);
	gpio_isr_handler_add(GPIO_INPUT_SWITCH_2, gpio_isr_handler,
						 (void *)GPIO_INPUT_SWITCH_2);

	// hook isr handler for specific gpio pin
	//     gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*)
	//     GPIO_INPUT_IO_1);

	// remove isr handler for gpio number.
	//gpio_isr_handler_remove(GPIO_INPUT_SWITCH);
	// hook isr handler for specific gpio pin again
	//gpio_isr_handler_add(GPIO_INPUT_SWITCH, gpio_isr_handler, (void *)GPIO_INPUT_SWITCH);
	ESP_LOGI(GPIO_TAG, "GPIO set up, turn everything off");
	setSwitchLed(off);
}


void setSwitchLed(bool onOff) {
	gpio_set_level(GPIO_OUTPUT_SWITCH_LED, onOff);
	switchLedStatus = onOff;
}

bool getSwitchLedStatus(void) { return switchLedStatus; }

welcher_Knopf_t getKnopfGedruecktReset() {
	welcher_Knopf_t tempKnopf = knopfGedrueckt;
	knopfGedrueckt = nicht;
	return tempKnopf;
}

welcher_Knopf_t getKnopfGedrueckt() {
	return knopfGedrueckt;
}
