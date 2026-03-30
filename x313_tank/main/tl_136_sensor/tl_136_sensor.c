/*
 * tl_136_sensor.c
 *
 *  Created on: 17.03.2026
 *      Author: al
 */
/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tl_136_sensor.h"
// #include "driver/adc_types_legacy.h"
#include "esp_err.h"
// #include "freertos/FreeRTOSConfig_arch.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/adc_types.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
// #include "hal/touch_sensor_types.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "portmacro.h"
// #include "sdkconfig.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "soc/soc_caps.h"

static adc_continuous_handle_t adc_handle;
// static adc_cali_handle_t adc1_cali_chan0_handle = NULL;

static TaskHandle_t tl136_cb_task = NULL; // freeRTOS taskhandle für ISR
static TaskHandle_t tl136_fluss_calc_task = NULL;
static TimerHandle_t tl136_fluss_timer = NULL;

static int adcSmoothData;
static int tankLiter;
static int tankProzent;
static int tankPromille;
static fliessRichtung_t wohin;

static bool IRAM_ATTR callback(adc_continuous_handle_t handle,
							   const adc_continuous_evt_data_t *edata,
							   void *user_data) {
	BaseType_t mustYield = pdFALSE;
	vTaskNotifyGiveFromISR(tl136_cb_task, &mustYield);
	return (mustYield == pdTRUE);
}

void tl136cbTask(void *parameters) {
	esp_err_t ret;
	uint32_t ret_num = 0;
	uint8_t result[TL_136_SAMPLES] = {0};
	memset(result, 0xcc, TL_136_SAMPLES);
	int milliVolt[TL_136_SAMPLES * 2];
	adc_cali_handle_t adc1_cali_chan0_handle = NULL;
	bool do_calibration1_chan0 =
		adc_calibration_init(TL_136_ADC_UNIT, TL_136_ADC_CHANNEL,
							 TL_136_ADC_ATTEN, &adc1_cali_chan0_handle);

	for (;;) {
		ulTaskNotifyTake(
			pdTRUE, portMAX_DELAY); // Wartet bis die ISR Routine bescheid gibt
		ret = adc_continuous_read(adc_handle, result, TL_136_SAMPLES, &ret_num,
								  0);
		if (ret == ESP_OK) {
			adc_continuous_data_t parsed_data[ret_num];
			uint32_t num_parsed_samples = 0;

			esp_err_t parse_ret = adc_continuous_parse_data(
				adc_handle, result, ret_num, parsed_data, &num_parsed_samples);
			if (parse_ret == ESP_OK) {
				uint32_t completeRawData = 0;
				for (int i = 0; i < num_parsed_samples; i++) {
					if (parsed_data[i].valid && do_calibration1_chan0) {
						ESP_ERROR_CHECK(adc_cali_raw_to_voltage(
							adc1_cali_chan0_handle, parsed_data[i].raw_data,
							&milliVolt[i]));
						completeRawData += milliVolt[i];

					} else {
						ESP_LOGW(
							TL_136_TAG, "Invalid data [ADC%d_Ch%d_%" PRIu32 "]",
							parsed_data[i].unit + 1, parsed_data[i].channel,
							parsed_data[i].raw_data);
					}
				}
				adcSmoothData = completeRawData / num_parsed_samples;
			} else {
				ESP_LOGE(TL_136_TAG, "Data parsing failed: %s",
						 esp_err_to_name(parse_ret));
			}

			vTaskDelay(100);
		} else if (ret == ESP_ERR_TIMEOUT) {
			ESP_LOGE(TL_136_TAG, "No Data");
			// We try to read `EXAMPLE_READ_LEN` until API returns timeout,
			// which means there's no available data
		}
	}
}

void flussCbTask(TimerHandle_t xFlussTimer) {
	// Fliessrichtung berechnen
	xTaskNotifyGive(tl136_fluss_calc_task);
}

void flussCalcTask(void *parameters) {
	static int counter;
	static int wasserStand[11]; // der 11te (#10) ist temp
	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		// schiebt alle Wasserstände eins weiter
		for (int i = 10; i > 0; i--) {
			wasserStand[i] = wasserStand[i - 1];
		}
		wasserStand[0] = getSmoothData(); // aktueller Stand in 0
		counter++;
		if (counter > 9) {
			counter = 0;
		}
		int diff = 0;
		for (int i = 1; i < 11; i++) {
			diff += wasserStand[i] - wasserStand[i - 1];
		}
		if (diff > 5) {
			wohin = raus;
		} else if (diff < -5) {
			wohin = rein;
		} else {
			wohin = still;
		}
	}
}

fliessRichtung_t getFlussRichtung(void) { return wohin; }

int getSmoothData(void) { return (adcSmoothData); }

int getTankProzent(void) {
	tankProzent = (adcSmoothData - TL_136_MIN) * 100 / TL_136_MAXMIN;
	if (tankProzent < 0) {
		tankProzent = 0;
	}
	return tankProzent;
}

int getTankPromille(void) {
	tankPromille = (adcSmoothData - TL_136_MIN) * 1000 / TL_136_MAXMIN;
	if (tankPromille < 0) {
		tankPromille = 0;
	}
	return tankPromille;
}

int getTankLiter(void) {
	tankLiter = (adcSmoothData - TL_136_MIN) * TL_136_LITRES / TL_136_MAXMIN;
	if (tankLiter < 0) {
		tankLiter = 0;
	}
	return tankLiter;
}

void ADC_Init(adc_channel_t *channels, uint8_t numChannels) {
	// handle configuration
	adc_continuous_handle_cfg_t handle_config = {
		.conv_frame_size = TL_136_SAMPLES,
		.max_store_buf_size = TL_136_SAMPLES * 2,
	};
	ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_config, &adc_handle));

	// ADC Configuration with Channels
	adc_continuous_config_t adc_cnfig = {.pattern_num = numChannels,
										 .conv_mode = ADC_CONV_SINGLE_UNIT_1,
										 .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
										 .sample_freq_hz = 20 * 1000};
	adc_digi_pattern_config_t channel_config[1];
	{
		channel_config[0].channel = TL_136_ADC_CHANNEL;
		channel_config[0].atten = TL_136_ADC_ATTEN;
		channel_config[0].bit_width = TL_136_ADC_BITWIDTH;
		channel_config[0].unit = TL_136_ADC_UNIT;
	}
	adc_cnfig.adc_pattern = channel_config;
	ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &adc_cnfig));

	// Callback Configuration
	adc_continuous_evt_cbs_t cb_config = {
		.on_conv_done = callback,
	};
	ESP_ERROR_CHECK(
		adc_continuous_register_event_callbacks(adc_handle, &cb_config, NULL));
}

void tl136_startup(void) {
	adc_channel_t tl136_channel[1] = {TL_136_ADC_CHANNEL};
	ADC_Init(tl136_channel, 1);
	ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

	xTaskCreate(tl136cbTask, "TL136 Callback Task", 4096, NULL, 4,
				&tl136_cb_task);
	xTaskCreate(flussCalcTask, "TL136 Fluss Task", 4096, NULL, 2,
				&tl136_fluss_calc_task);
	tl136_fluss_timer = xTimerCreate("FlussTimer", pdMS_TO_TICKS(500), pdTRUE,
									 NULL, flussCbTask);
	/* Die Pumpe pumpt nicht so schnell. Also alle 3s messen, insg. 30s.
	* gibt also aus ob die letzte halbe Minute aufgefüllt wurde oder 
	* geleert.
	*/
	xTimerStart(tl136_fluss_timer, pdMS_TO_TICKS(3000));
}

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
						  adc_atten_t atten, adc_cali_handle_t *out_handle) {
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

	if (!calibrated) {
		ESP_LOGI(TL_136_TAG, "calibration scheme version is %s",
				 "Line Fitting");
		adc_cali_line_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}

	*out_handle = handle;
	if (ret == ESP_OK) {
		ESP_LOGI(TL_136_TAG, "Calibration Success");
	} else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
		ESP_LOGW(TL_136_TAG, "eFuse not burnt, skip software calibration");
	} else {
		ESP_LOGE(TL_136_TAG, "Invalid arg or no memory");
	}
	return calibrated;
}

// static void adc_calibration_deinit(adc_cali_handle_t handle) {
//	ESP_LOGI(TL_136_TAG, "deregister %s calibration scheme", "Line Fitting");
//	ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
// }
