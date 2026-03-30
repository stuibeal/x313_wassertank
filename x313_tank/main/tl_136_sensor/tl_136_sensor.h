// #pragma once
/*
 * tl_136_sensor.h
 *
 *  Created on: 17.03.2026
 *      Author: al
 */

#ifndef TL_136_SENSOR_H_
#define TL_136_SENSOR_H_

#include <stdint.h>
#include <sys/unistd.h>
// #include "driver/adc_types_legacy.h"
// #include "freertos/idf_additions.h"
#include "hal/adc_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
// #include "freertos/semphr.h"
// #include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
// #include "esp_adc/adc_cali_scheme.h"

#define TL_136_ADC_CHANNEL ADC_CHANNEL_6
#define TL_136_ADC_ATTEN ADC_ATTEN_DB_12
#define TL_136_ADC_BITWIDTH ADC_BITWIDTH_12
#define TL_136_ADC_UNIT ADC_UNIT_1

/** Sensor mit 150 Ohm an Ground und an ADC pin gezogen
 * - heisst: 0,6 V bei 4mA (leer)
 *   	    3,0 V bei 20mA (1m Wassersäule)
 * ergo: Tankhöhe 50cm ca
 * also: 4mA + 8mA = 12mA
 * also U=R*I: 150*0,012 = 1,8V
 * Mitte: 1800mV - 600mV = 1200mV, /2: 600mV +600mV(leer) = 1200mV Mitte
 * Spannungsteiler an ADC7 (pin 36) (1200mV/2100mV) (0,36:0,63 -> ~1:2 ) 3v3 -
 *80K - ADC7- 47K -GND
 **/
// 4mA = 0,6V, (3300mv / 4096 *600mV)  460-1620 558-1700  Tank voll->leer ca
// 1142-1160

// Minimum in mV (zum benutzen mit calibration function)
#define TL_136_MIN 530

// Maximium in mV
// 12mA = 1,8V (3300mv / 4096 * 1800mV)
#define TL_136_MAX 1500
// Tankgröße
#define TL_136_LITRES 350
// max daten
#define TL_136_MAX_DATA 24
#define TL_136_TAG "TL136"

#define TL_136_MAXMIN (TL_136_MAX - TL_136_MIN)
#define TL_136_SAMPLES 64

typedef enum {
	still,
	rein,
	raus,
} fliessRichtung_t;

fliessRichtung_t getFlussRichtung(void);

int getSmoothData(void);
int getTankProzent(void);
int getTankPromille(void);
int getTankLiter(void);
void tl136cbTask(void *parameters); // callback Task zum ausrechnen wenn fertig
void flussCbTask(TimerHandle_t xFlussTimer);
void flussCalcTask(void *parameters);
void ADC_Init(adc_channel_t *channels, uint8_t numChannels);
void tl136_startup(void); // Tasks einrichten etc
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
						  adc_atten_t atten, adc_cali_handle_t *out_handle);
// static void adc_calibration_deinit(adc_cali_handle_t handle);

#endif /* TL_136_SENSOR_H_ */
