/*
Set up all necessary GPIO pins
*/
#ifndef GPIO_HANDLER_H_
#define GPIO_HANDLER_H_

#define ESP_INTR_FLAG_DEFAULT 0

#define GPIO_INPUT_SWITCH 4 // am 2er hängt die doofe Onboard LED
#define GPIO_INPUT_SWITCH_2 18
#define GPIO_OUTPUT_PUMPE 16
#define GPIO_OUTPUT_VENTIL 17
#define GPIO_OUTPUT_SWITCH_LED 15
#define on 1
#define off 0
#define GPIO_TAG "GPIO_HANDLER"

#endif /* PIN_H_ */

// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
// #include "driver/gpio.h"
// #include "hal/gpio_types.h"

// static void IRAM_ATTR gpio_isr_handler(void *arg);
// static void gpio_task(void *arg);

typedef enum { nicht, links, rechts, } welcher_Knopf_t;

void gpio_Task(void *arg);
void setup_gpio_pins(void);

void setSwitchLed(bool onOff);
bool getSwitchLedStatus(void);

welcher_Knopf_t getKnopfGedruecktReset(void);
welcher_Knopf_t getKnopfGedrueckt(void);
