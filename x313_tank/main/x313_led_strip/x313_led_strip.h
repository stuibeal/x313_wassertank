#pragma once
/*
 * led_strip.h
 *
 *  Created on: 19.03.2026
 *      Author: al
 */

#ifndef MAIN_X313_LED_STRIP_X313_LED_STRIP_H_
#define MAIN_X313_LED_STRIP_X313_LED_STRIP_H_
#include "led_strip.h"
#include <stdbool.h>

// GPIO assignment
#define LED_STRIP_GPIO_PIN 19
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 24
// Regenbogen
#define WS2812B_FRAME_DURATION_MS 2
#define WS2812B_ANGLE_INC_FRAME 0.02
#define WS2812B_ANGLE_INC_LED 0.3
#define LEDSTRIP_TAG "LED_STRIP"

typedef enum {
	init,
	ledAus,
	licht,
	wasserstand,
	regenbogen,
	kreisel,
	warnung,
} ledStatus_t;

typedef struct {
	int R;
	int G;
	int B;
} ledColor_t;

static const ledColor_t weiss = {255, 255, 255};
static const ledColor_t blau = {0, 0, 255};
static const ledColor_t rot = {255, 0, 0};
static const ledColor_t mohm = {12, 34, 56};
static const ledColor_t gelb = {0xF9, 0xA6, 0x05};
static const ledColor_t violett = {0xCC, 0x33, 0xCC};

typedef struct {
	ledStatus_t ledZustand;
	int wasserStandDaten;
	int helligkeit;
	bool btOn;
	ledColor_t farbe;
} datenFuerLed_t;

void ledTask(void *arg);

led_strip_handle_t configure_led(void);

void ledAusschalten(bool mitStart);
void ledWasserstand(bool mitStart);
void ledWasserstandLight(bool mitStart); 
void ledRegenbogen(bool mitStart);
void ledBeleuchtung(bool mitStart);
void ledKreisel(bool mitStart);
void ledWarnung(bool mitStart);

void led_strip_startup(void);

void setPixelWithBrightness(uint32_t led, uint32_t r, uint32_t g, uint32_t b);
void setPixelColorBrightness(uint32_t led, uint32_t brightness);
void setStripColorBrightness(uint32_t brightness);

ledStatus_t getLedStatus(void);
int getLichtHelligkeit(void);
void setLedStatus(ledStatus_t status, ledColor_t farbe);
void setLichtHelligkeit(int hell);
void setBeleuchtung(bool onOff);
bool getBeleuchtung(void);
void setWasserstand(int promille);
void setBtLed(bool onOff);
#endif /* MAIN_X313_LED_STRIP_X313_LED_STRIP_H_ */
