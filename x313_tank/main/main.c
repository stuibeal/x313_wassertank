/*
 * Wassertank Anzeige für das Friedshofsmobil
 *
 * V 0.1 30.3.26
  */


#include "bluetooth_spp/bluetooth_spp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "gpio_handler/gpio_handler.h"
#include "pumpe.h"
#include "time.h"
#include "tl_136_sensor/tl_136_sensor.h"
#include "valve.h"
#include "x313_led_strip/x313_led_strip.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SERVER_TAG "X313_SPP_SERVER"

// Queues
QueueHandle_t btDataQueue;

void checkTask(void *parameters) {
	// Check Bluetooth Data
	btData_t data;
	for (;;) {

		if (xQueueReceive(btDataQueue, &data, portMAX_DELAY) == pdTRUE) {
			switch (data.firstByte) {
			case pumpe:
				ESP_LOGI(SERVER_TAG, "BT Pumpe %d", data.data);
				setPumpeHand(data.data);
				break;
			case ventil:
				ESP_LOGI(SERVER_TAG, "BT Ventil %d", data.data);
				setVentilHand(data.data);
				break;
			case helligkeit:
				ESP_LOGI(SERVER_TAG, "BT Helligkeit %d", data.data);
				if (data.data < 256) {
					setLichtHelligkeit(data.data);
				} else {
					setLichtHelligkeit(255);
				}
				break;
			case beleuchtung:
				ESP_LOGI(SERVER_TAG, "BT Beleuchtung %d", data.data);
				if (data.data > 0) {
					setBeleuchtung(on);
				} else {
					setBeleuchtung(off);
				}
				break;
			case bt_connection:
				setBtLed(data.data);
				break;
			default:
				ESP_LOGI(SERVER_TAG,
						 "FirstByte %d -%d-%d-%d-%d-%d- len %d data %d",
						 data.firstByte, data.rohdaten[0], data.rohdaten[1],
						 data.rohdaten[2], data.rohdaten[3], data.rohdaten[4],
						 data.len, data.data);
				break;
			} // switch
		} // if bt
		vTaskDelay(pdMS_TO_TICKS(20));
	} // for
} // void

void app_main(void) {
	// esp_log_level_set("*", ESP_LOG_ERROR);
	char buffer[128];

	// Startup setup
	ESP_LOGI(SERVER_TAG, "Startup app_main");
	ESP_LOGI(SERVER_TAG, "GPIO start");
	setup_gpio_pins();
	ESP_LOGI(SERVER_TAG, "BT Stack start");
	bt_startup();
	ESP_LOGI(SERVER_TAG, "TL-136 Sensor start");
	tl136_startup();
	ESP_LOGI(SERVER_TAG, "Create Tasks");
	static TaskHandle_t ledTaskHandle;	 // WS2812B LED STRIP
	static TaskHandle_t valveTaskHandle; // Ventil
	static TaskHandle_t pumpeTaskHandle; // Pumpe
	static TaskHandle_t checkTaskHandle; // BT und Rest

	xTaskCreate(ledTask, "Led Task", 4096, NULL, 1, &ledTaskHandle);
	xTaskCreate(valveTask, "Valve Task", 4096, NULL, 1, &valveTaskHandle);
	xTaskCreate(pumpeTask, "Pumpe Task", 4096, NULL, 1, &pumpeTaskHandle);
	xTaskCreate(checkTask, "Check Task", 4096, NULL, 1, &checkTaskHandle);
	ESP_LOGI(SERVER_TAG, "Start Task Scheduler");

	// Startup Wasserstand
	setWasserstand(getTankPromille());
	setLichtHelligkeit(255);
	setLedStatus(regenbogen, mohm);
	vTaskDelay(pdMS_TO_TICKS(1000));
	setLedStatus(wasserstand, mohm);
	vTaskDelay(pdMS_TO_TICKS(200));
	int oldTankPromille = getTankPromille();

	ESP_LOGI(SERVER_TAG, "Verweile");

	while (1) {
		// check Wasserstand und so
			if (abs(oldTankPromille - getTankPromille()) > 2) {
				setWasserstand(getTankPromille());
				oldTankPromille = getTankPromille();
			}

			int laenge = sprintf(
				buffer, "S%04i-P%04i-L%03i-PU%i-VE%i-LI%i-HE%03i-K%i-F%i-B%i\r\n",
				getSmoothData(), getTankPromille(), getTankLiter(),
				getPumpeStatus(), getVentil(), getLedStatus(),
				getLichtHelligkeit(), getKnopfGedrueckt(),getFlussRichtung(), getBeleuchtung());
			if (laenge > 10) {
				sendBTMessage(buffer);
			}
			// ESP_LOGI(SERVER_TAG, "%s", buffer);

			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
