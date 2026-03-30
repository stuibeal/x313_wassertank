/*
 * led_strip.c
 *
 *  Created on: 19.03.2026
 *      Author: al
 */
#include "x313_led_strip.h"
#include "esp_err.h"
#include "esp_log.h"
// #include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "LED_WS2812B";
static led_strip_handle_t led_strip;
static bool beleuchtungErwuenscht = 0;
static datenFuerLed_t ist;
static datenFuerLed_t soll;

QueueHandle_t led_evt_queue = NULL;

// datenFuerLed_t ist;
// datenFuerLed_t soll;

void ledTask(void *arg) {
	led_strip = configure_led();
	bool newState = 1;
	soll.ledZustand = regenbogen;
	for (;;) {
		// only one time (bei änderung)
		if (soll.ledZustand != ist.ledZustand) {
			newState = 1;
		}
		switch (soll.ledZustand) {
		case init:
			break;
		case ledAus:
			ledAusschalten(newState);
			break;
		case licht:
			ledBeleuchtung(newState);
			break;
		case wasserstand:
			if (beleuchtungErwuenscht) {
				ledWasserstandLight(newState);
			} else {
				ledWasserstand(newState);
			}
			break;
		case regenbogen:
			ledRegenbogen(newState);
			break;
		case kreisel:
			ledKreisel(newState);
			break;
		case warnung:
			ledWarnung(newState);
			break;
		default:
			break;
		} // SWITCH
		ist.ledZustand = soll.ledZustand;
		newState = 0;
		vTaskDelay(10);
	} // FOR
} // VOID

led_strip_handle_t configure_led(void) {
	// LED strip general initialization, according to your led board design
	led_strip_config_t strip_config = {
		.strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the
											  // LED strip's data line
		.max_leds = LED_STRIP_LED_COUNT, // The number of LEDs in the strip,
		.led_model = LED_MODEL_WS2812,	 // LED strip model
		// set the color order of the strip: GRB
		.color_component_format =
			{
				.format =
					{
						.r_pos = 1, // red is the second byte in the color data
						.g_pos = 0, // green is the first byte in the color data
						.b_pos = 2, // blue is the third byte in the color data
						.num_components = 3, // total 3 color components
					},
			},
		.flags = {
			.invert_out = false, // don't invert the output signal
		}};

	// LED strip backend configuration: SPI
	led_strip_spi_config_t spi_config = {
		.clk_src = SPI_CLK_SRC_DEFAULT, // different clock source can lead to
										// different power consumption
		.spi_bus = SPI2_HOST,			// SPI bus ID
		.flags = {
			.with_dma = true, // Using DMA can improve performance and help
							  // drive more LEDs
		}};

	// LED Strip object handle
	led_strip_handle_t led_strip;
	ESP_ERROR_CHECK(
		led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
	ESP_LOGI(TAG, "Created LED strip object with SPI backend");
	return led_strip;
}

void ledAusschalten(bool mitStart) {
	ESP_LOGI(LEDSTRIP_TAG, "LED aus");
	ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

void ledWasserstand(bool mitStart) {
	if (mitStart) {
		ESP_LOGI(LEDSTRIP_TAG, "Wasserstand start");
	}
	// Bluetooth
	setPixelWithBrightness(0, 0, 0, soll.btOn * soll.helligkeit);
	setPixelWithBrightness(23, 0, 0, soll.btOn * soll.helligkeit);
	// LED 2 ROT
	setPixelWithBrightness(1, 255, 0, 0);
	// LED 23 GRÜN
	setPixelWithBrightness(22, 0, 255, 0);
	//
	//	if (soll.btOn != ist.btOn) {
	//		ESP_LOGI(LEDSTRIP_TAG, "Bluetoothanzeige");
	//		setPixelWithBrightness(0, 0, 0, soll.btOn * soll.helligkeit);
	//		setPixelWithBrightness(23, 0, 0, soll.btOn * soll.helligkeit);
	//		ESP_ERROR_CHECK(led_strip_refresh(led_strip)); // Alle aktualisieren
	//		ist.btOn = soll.btOn;
	//	}
	if (soll.wasserStandDaten != ist.wasserStandDaten) {
		ESP_LOGI(LEDSTRIP_TAG, "Wasserstand aktualisieren");
		int actPixBright = 0;
		int actR = 0;
		int actG = 0;
		int actB = 0;
		int restPromille = soll.wasserStandDaten; // jede LED macht 50 Promille
		if (restPromille > 1000) {
			restPromille = 1000;
		}
		for (int i = 0; i < 20; i++) {
			if (restPromille > 49) {
				actPixBright = soll.helligkeit;
				restPromille -= 50;
			} else {
				actPixBright = soll.helligkeit * restPromille / 50;
				restPromille = 0;
			}
			if (i < 4) {
				actR = actPixBright; // Fade in bei rot
				actG = actPixBright / (5 - i);
				actB = actPixBright / (5 - i);
			} else if (i > 15) {
				actR = actPixBright / (i - 15); // Fade out bei grün
				actG = actPixBright;
				actB = actPixBright / (i - 15);
			} else {
				actR = actPixBright;
				actG = actPixBright;
				actB = actPixBright;
			}
			setPixelWithBrightness(i + 2, actR, actG, actB);
		}
		ESP_ERROR_CHECK(led_strip_refresh(led_strip)); // Alle aktualisieren
		ist.wasserStandDaten = soll.wasserStandDaten;
	}
}

void ledWasserstandLight(bool mitStart) {
	if (mitStart) {
		ESP_LOGI(LEDSTRIP_TAG, "Wasserstand start");
	}
	setPixelWithBrightness(0, 0, 0, soll.btOn * soll.helligkeit);
	setPixelWithBrightness(23, 0, 0, soll.btOn * soll.helligkeit);
	// LED 2 ROT
	setPixelWithBrightness(1, 255, 0, 0);
	// LED 23 GRÜN
	setPixelWithBrightness(22, 0, 255, 0);
	//	if (soll.btOn != ist.btOn) {
	//		ESP_LOGI(LEDSTRIP_TAG, "Bluetoothanzeige");
	//		setPixelWithBrightness(0, 0, 0, soll.btOn * soll.helligkeit);
	//		setPixelWithBrightness(23, 0, 0, soll.btOn * soll.helligkeit);
	//		ESP_ERROR_CHECK(led_strip_refresh(led_strip)); // Alle aktualisieren
	//		ist.btOn = soll.btOn;
	//	}
	if (soll.wasserStandDaten != ist.wasserStandDaten) {
		ESP_LOGI(LEDSTRIP_TAG, "Wasserstand aktualisieren");
		// erstmal alle uplighten
		for (int i = 2; i < 22; i++) {
			setPixelWithBrightness(i, 255, 255, 255);
		}
		// ein Pixel zeigt den Wasserstand
		for (int i = 0; i < 20; i++) {
			if (soll.wasserStandDaten + 1 > i * 50 &&
				soll.wasserStandDaten - 1 < (i + 1) * 50) {
				setPixelWithBrightness(i + 2, 0, 0, 255);
			}
		}
		ESP_ERROR_CHECK(led_strip_refresh(led_strip)); // Alle aktualisieren
		ist.wasserStandDaten = soll.wasserStandDaten;
	}
}

void ledRegenbogen(bool mitStart) {
	if (mitStart) {
		ESP_LOGI(LEDSTRIP_TAG, "Regenbogen");
	}
	static float offset = 0;
	for (int led = 0; led < LED_STRIP_LED_COUNT; led++) {
		// Build RGB pixels. Each color is an offset sine, which
		// gives a hue-like effect.
		float angle = offset + (led * WS2812B_ANGLE_INC_LED);
		const float color_off = (M_PI * 2) / 3;
		//		ESP_ERROR_CHECK(led_strip_set_pixel(
		//			led_strip, led, sin(angle + color_off * 0) * 127 + 128,
		//			sin(angle + color_off * 1) * 127 + 128,
		//			sin(angle + color_off * 2) * 117 + 128));
		setPixelWithBrightness(led, sin(angle + color_off * 0) * 127 + 128,
							   sin(angle + color_off * 1) * 127 + 128,
							   sin(angle + color_off * 2) * 117 + 128);
	}
	ESP_ERROR_CHECK(led_strip_refresh(led_strip)); // Alle aktualisieren
	vTaskDelay(pdMS_TO_TICKS(WS2812B_FRAME_DURATION_MS));
	// Increase offset to shift pattern
	offset += WS2812B_ANGLE_INC_FRAME;
	if (offset > 2 * M_PI) {
		offset -= 2 * M_PI;
	}
}

void ledBeleuchtung(bool mitStart) {
	if (mitStart) {
		ESP_LOGI(LEDSTRIP_TAG, "Licht an");
		soll.farbe = weiss;
		setStripColorBrightness(255);
	}
	if (soll.helligkeit != ist.helligkeit) {
		setStripColorBrightness(255);
		ist.helligkeit = soll.helligkeit;
	}
}

void ledKreisel(bool mitStart) {
	static int kreiselLed;
	if (mitStart) {
		ESP_LOGI(LEDSTRIP_TAG, "LED Kreisel");
		ESP_ERROR_CHECK(led_strip_clear(led_strip));
		kreiselLed = 0;
	}
	for (int lednum = 0; lednum < 9; lednum++) {
		kreiselLed++;
		if (kreiselLed > LED_STRIP_LED_COUNT - 1) {
			kreiselLed = 0;
		}
		setPixelColorBrightness(kreiselLed, lednum * 32);
	}
	kreiselLed -= 8; // wir hatten 9 Durchläufe, 8 ziehen wir wieder ab,
					 // dadurch eine Stelle weiter
	if (kreiselLed < 0) {
		kreiselLed += LED_STRIP_LED_COUNT; // wenn im Minus sind
	}
	ESP_ERROR_CHECK(led_strip_refresh(led_strip)); // Alle aktualisieren
	vTaskDelay(pdMS_TO_TICKS(5));
}

void ledWarnung(bool mitStart) {
	if (mitStart) {
		ESP_LOGI(LEDSTRIP_TAG, "Warnung");
	}
	static int brightness;
	static bool up;
	if (mitStart) {
		brightness = 0;
		up = 1;
	}
	if (brightness < 252 && up) {
		brightness += 4;
	} else {
		up = 0;
	}
	if (brightness > 7 && !up) {
		brightness -= 8;
	} else {
		up = 1;
	}
	setStripColorBrightness(brightness);
	vTaskDelay(pdMS_TO_TICKS(5));
}

void setPixelWithBrightness(uint32_t led, uint32_t r, uint32_t g, uint32_t b) {
	ESP_ERROR_CHECK(led_strip_set_pixel(
		led_strip, led, r * soll.helligkeit / 255, g * soll.helligkeit / 255,
		b * soll.helligkeit / 255));
}
void setPixelColorBrightness(uint32_t led, uint32_t brightness) {
	uint32_t r = brightness * soll.farbe.R * soll.helligkeit / 65025;
	uint32_t g = brightness * soll.farbe.G * soll.helligkeit / 65025;
	uint32_t b = brightness * soll.farbe.B * soll.helligkeit / 65025;
	ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, led, r, g, b));
}

void setStripColorBrightness(uint32_t brightness) {
	for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
		setPixelColorBrightness(i, brightness);
	}
	ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

ledStatus_t getLedStatus(void) { return ist.ledZustand; };
int getLichtHelligkeit(void) { return soll.helligkeit; };
void setLedStatus(ledStatus_t status, ledColor_t farbe) {
	soll.ledZustand = status;
	soll.farbe = farbe;
};
void setLichtHelligkeit(int hell) { soll.helligkeit = hell; };
void setBeleuchtung(bool onOff) { beleuchtungErwuenscht = onOff; };
bool getBeleuchtung(void) { return beleuchtungErwuenscht; };
void setWasserstand(int promille) { soll.wasserStandDaten = promille; };
void setBtLed(bool onOff) { soll.btOn = onOff; }
