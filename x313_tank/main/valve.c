/*
 * valve.c
 *
 *  Created on: 25.03.2026
 *      Author: al
 */
#include "valve.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "gpio_handler/gpio_handler.h"
#include "sys/time.h"
#include "time.h"
#include "tl_136_sensor/tl_136_sensor.h"
#include "x313_led_strip/x313_led_strip.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool ventilStatus;
static bool ventilHand;
static valveStatus_t vStatus;

void valveTask(void *parameters) {
	ventilStatus = 0;
	ventilHand = 0;
	vStatus = vWiederOk;

	int valvePromille = getTankPromille();
	static struct timeval time_new, time_old;
	vStatus = vWiederOk; // on Startup
	for (;;) {
		switch (vStatus) {
		case vAllesOk:
			if (getTankPromille() > MAX_WASSERSTAND_PROMILLE) {
				vStatus = vZuVielAnfang;
			}
			break;
		case vZuVielAnfang:
			ESP_LOGI("VENTIL", "Zu viel Anfang");
			setVentil(off);					// Ventil aus
			setSwitchLed(on);				// Schalterlicht ein
			setLedStatus(regenbogen, mohm); // LEDs auf Regenbogen
			vStatus = vZuVielWarten;
			break;
		case vZuVielWarten:
			if (getTankPromille() <
				MAX_WASSERSTAND_PROMILLE - VENTIL_HYSTERESE) {
				vStatus = vWiederOk;
			}
			if (getKnopfGedruecktReset() > nicht || ventilHand) {
				vStatus = vHandEin;
				//ventilHand = 0;
			}
			break;
		case vWiederOk:
			ESP_LOGI("VENTIL", "Wieder ok");
			setVentil(on);
			setSwitchLed(off);
			setLedStatus(wasserstand, mohm);
			vStatus = vAllesOk;
			ventilHand = 0;
			break;
		case vHandEin:
			ESP_LOGI("VENTIL", "Hand ein");
			setLedStatus(kreisel, blau);
			setSwitchLed(off);
			setVentil(on);
			vStatus = vHandEinWarten;
			valvePromille = getTankPromille();
			gettimeofday(&time_old, NULL);
			break;
		case vHandEinWarten:
			if ((getTankPromille() - valvePromille > VENTIL_HYSTERESE_HAND) ||
				getTankPromille() > MAX_WASSERSTAND_ABSOLUT) {
				vStatus = vHandUeberfuellt;
			}
			if (!ventilHand && ventilStatus) {
				vStatus = vZuVielAnfang;
			}
	
			// Nach 30 sec oder Wasserstand ok wieder in normalen Modus zurück
			gettimeofday(&time_new, NULL);
			if (time_new.tv_sec - time_old.tv_sec > 30 || getTankPromille() < MAX_WASSERSTAND_PROMILLE - VENTIL_HYSTERESE_HAND) {
				vStatus = vZuVielAnfang;
			}
			
			break;
		case vHandUeberfuellt:
		ESP_LOGI("VENTIL", "Hand überfüllt");
			setSwitchLed(on);
			setVentil(off);
			setLedStatus(warnung, violett);
			vStatus = vZuVielWarten;
			ventilHand = 0;
			break;
		default:
			break;
		} // Switch
		vTaskDelay(pdMS_TO_TICKS(30));
	} // FOR
}

void setVentil(bool onOff) {
	gpio_set_level(GPIO_OUTPUT_VENTIL, onOff);
	ventilStatus = onOff;
}

void setVentilHand(bool onOff) { ventilHand = onOff; };
bool getVentilHand(void) { return ventilHand; }
bool getVentil(void) { return ventilStatus; };
valveStatus_t getVentilStatus(void) { return vStatus; };
