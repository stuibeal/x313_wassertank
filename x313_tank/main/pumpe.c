/*
 * pumpe.c
 *
 *  Created on: 25.03.2026
 *      Author: al
 */
#include "pumpe.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "gpio_handler/gpio_handler.h"
// #include "sys/time.h"
// #include "time.h"
#include "tl_136_sensor/tl_136_sensor.h"
#include "x313_led_strip/x313_led_strip.h"
#include <stdbool.h>

static pumpenStatus_t pStatus;
static bool pumpeStatus;
static bool pumpeHand;

void pumpeTask(void *arg) {
	pStatus = beginnAus;
	pumpeStatus = 0;
	pumpeHand = 0;
	for (;;) {
		switch (pStatus) {
		case beginnAus:
			ESP_LOGI(PUMPEN_TAG, "Pumpe beginn");
			setPumpe(off);
			pStatus = anschalten;
			break;
		case anschalten:
			ESP_LOGI(PUMPEN_TAG, "Pumpe anschalten");

			if (getTankPromille() > AUS_WASSERSTAND_PROMILLE) {
				setPumpe(on);
			}
			setLedStatus(wasserstand, mohm);
			pStatus = normalAn;
			break;
		case normalAn:
			// ESP_LOGI(PUMPEN_TAG, "Pumpe normal an");

			if (getTankPromille() < WARN_WASSERSTAND_PROMILLE && getFlussRichtung()== raus) {
				pStatus = warnen;
			}
			if (pumpeHand) {
				pStatus = handEin;
			}
			break;
		case warnen:
			ESP_LOGI(PUMPEN_TAG, "Warnung: wenig");
			setLedStatus(warnung, gelb);
			for (int i = 0; i < 4; i++) {
				setPumpe(off);
				vTaskDelay(pdMS_TO_TICKS(200)); 
				setPumpe(on);
				vTaskDelay(pdMS_TO_TICKS(500));
			}
			setLedStatus(wasserstand, mohm);
			pStatus = warnenWarten;
			break;
		case warnenWarten:
			if (getTankPromille() < AUS_WASSERSTAND_PROMILLE) {
				pStatus = zuWenigAnfang;
			}
			if (getTankPromille() + PUMPEN_HYSTERESE >
				WARN_WASSERSTAND_PROMILLE) {
				pStatus = anschalten;
			}
			break;
		case zuWenigAnfang:
			ESP_LOGI(PUMPEN_TAG, "Warnung: zu Wenig (begin)");

			setLedStatus(warnung, rot);
			setPumpe(off);
			pStatus = zuWenigWarten;
			break;
		case zuWenigWarten:
			if (getTankPromille() + PUMPEN_HYSTERESE >
				AUS_WASSERSTAND_PROMILLE) {
				pStatus = anschalten;
			}
			if (getTankPromille() < AUS_WASSERSTAND_PROMILLE) {
				pStatus = zuWenigWarten;
			}
			if (pumpeHand) {
				pStatus = handEin;
			}
			break;
		case handEin:
			setLedStatus(warnung, mohm);
			setPumpe(!getPumpeStatus());
			pumpeHand = 0;
			pStatus = handWarten;
			break;
		case handWarten:
			if (pumpeHand) {
				pStatus = anschalten;
				pumpeHand = 0;
			}
			break;
		default:
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

void setPumpe(bool onOff) {
	gpio_set_level(GPIO_OUTPUT_PUMPE, onOff);
	pumpeStatus = onOff;
}

bool getPumpeStatus(void) { return pumpeStatus; };
bool getPumpeHand(void) { return pumpeHand; };
void setPumpeHand(bool onOff) { pumpeHand = onOff; };
