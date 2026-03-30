/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 * modified Al 17.3.26
 */

#include "bluetooth_spp.h"
#include <stdio.h>

#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "esp_spp_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "gpio_handler/gpio_handler.h"
#include "portmacro.h"
#include "sys/time.h"
#include "time.h"

static spp_conn_t s_conn = {
	.handle = 0, .connected = false, .congested = false};

static char *bda2str(uint8_t *bda, char *str, size_t size) {
	if (bda == NULL || str == NULL || size < 18) {
		return NULL;
	}

	uint8_t *p = bda;
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4],
			p[5]);
	return str;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
	btData_t data = {}; // NULL
	char bda_str[18] = {0};
	switch (event) {
	case ESP_SPP_INIT_EVT:
		if (param->init.status == ESP_SPP_SUCCESS) {
			ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
			esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
		} else {
			ESP_LOGE(SPP_TAG, "ESP_SPP_INIT_EVT status:%d", param->init.status);
		}
		break;
	case ESP_SPP_DISCOVERY_COMP_EVT:
		ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
		break;
	case ESP_SPP_OPEN_EVT:
		ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
		break;
	case ESP_SPP_CLOSE_EVT:
		s_conn.connected = false;
		s_conn.handle = 0;
		data.firstByte = bt_connection; //für LED
		data.data = false;
		xQueueSend(btDataQueue, &data, portMAX_DELAY);

		ESP_LOGI(SPP_TAG,
				 "ESP_SPP_CLOSE_EVT status:%d handle:%" PRIu32
				 " close_by_remote:%d",
				 param->close.status, param->close.handle, param->close.async);
		break;
	case ESP_SPP_START_EVT:
		if (param->start.status == ESP_SPP_SUCCESS) {
			ESP_LOGI(SPP_TAG,
					 "ESP_SPP_START_EVT handle:%" PRIu32 " sec_id:%d scn:%d",
					 param->start.handle, param->start.sec_id,
					 param->start.scn);
			esp_bt_gap_set_device_name(local_device_name);
			esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,
									 ESP_BT_GENERAL_DISCOVERABLE);
		} else {
			ESP_LOGE(SPP_TAG, "ESP_SPP_START_EVT status:%d",
					 param->start.status);
		}
		break;
	case ESP_SPP_CL_INIT_EVT:
		ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
		break;
	case ESP_SPP_DATA_IND_EVT:
		ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len:%d handle:%" PRIu32,
				 param->data_ind.len, param->data_ind.handle);
		data.firstByte = param->data_ind.data[0];
		uint32_t stellen = 0;
		for (int i = 0; (i < param->data_ind.len) && i < 10; i++) {
			data.rohdaten[i] = param->data_ind.data[i + 1];
			if (data.rohdaten[i] > 47 && data.rohdaten[i] < 58) {
				stellen++;
			}
		}
		/* Umwandlung in int - ohne Vorzeichen,  pow ist wurzel 10 hoch dings */
		for (int i = 0; i < stellen; i++) {
			data.data += (data.rohdaten[i] - 48) * pow(10, stellen - i - 1);
		}
		data.len = param->data_ind.len;
		ESP_LOGI(SPP_TAG, "f: %d d: %d s: %d", data.firstByte, data.data,
				 stellen);
		xQueueSend(btDataQueue, &data, portMAX_DELAY);

		break;
	case ESP_SPP_CONG_EVT:
		ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
		s_conn.congested = true;
		break;
	case ESP_SPP_WRITE_EVT:
		if (param->write.cong) {
			s_conn.congested = true;
		}
		// ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
		break;
	case ESP_SPP_SRV_OPEN_EVT:
		ESP_LOGI(SPP_TAG,
				 "ESP_SPP_SRV_OPEN_EVT status:%d handle:%" PRIu32
				 ", rem_bda:[%s]",
				 param->srv_open.status, param->srv_open.handle,
				 bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));
		s_conn.handle = param->srv_open.handle;
		s_conn.connected = true;
		s_conn.congested = false;
		const char *welcome_msg = "Willkommen beim ROD-X313 SPP Server!\r\n";
		esp_spp_write(param->srv_open.handle, strlen(welcome_msg),
					  (uint8_t *)welcome_msg);
	    data.firstByte = bt_connection; //für LED
		data.data = true;
		xQueueSend(btDataQueue, &data, portMAX_DELAY);
		break;
	case ESP_SPP_SRV_STOP_EVT:
		ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_STOP_EVT");
		break;
	case ESP_SPP_UNINIT_EVT:
		ESP_LOGI(SPP_TAG, "ESP_SPP_UNINIT_EVT");
		break;
	default:
		break;
	}
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
	char bda_str[18] = {0};

	switch (event) {
	case ESP_BT_GAP_AUTH_CMPL_EVT: {
		if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
			ESP_LOGI(SPP_TAG, "authentication success: %s bda:[%s]",
					 param->auth_cmpl.device_name,
					 bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
		} else {
			ESP_LOGE(SPP_TAG, "authentication failed, status:%d",
					 param->auth_cmpl.stat);
		}
		break;
	}
	case ESP_BT_GAP_PIN_REQ_EVT: {
		ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d",
				 param->pin_req.min_16_digit);
		if (param->pin_req.min_16_digit) {
			ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
			esp_bt_pin_code_t pin_code = {0};
			esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
		} else {
			ESP_LOGI(SPP_TAG, "Input pin code: 1234");
			esp_bt_pin_code_t pin_code;
			pin_code[0] = '1';
			pin_code[1] = '2';
			pin_code[2] = '3';
			pin_code[3] = '4';
			esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
		}
		break;
	}

#if (CONFIG_EXAMPLE_SSP_ENABLED == true)
	case ESP_BT_GAP_CFM_REQ_EVT:
		ESP_LOGI(SPP_TAG,
				 "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: "
				 "%06" PRIu32,
				 param->cfm_req.num_val);
		esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
		break;
	case ESP_BT_GAP_KEY_NOTIF_EVT:
		ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%06" PRIu32,
				 param->key_notif.passkey);
		break;
	case ESP_BT_GAP_KEY_REQ_EVT:
		ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
		break;
#endif

	case ESP_BT_GAP_MODE_CHG_EVT:
		ESP_LOGI(SPP_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d bda:[%s]",
				 param->mode_chg.mode,
				 bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
		break;

	default: {
		ESP_LOGI(SPP_TAG, "event: %d", event);
		break;
	}
	}
	return;
}

void sendBTMessage(char *message) {
	if (s_conn.connected && !s_conn.congested) {
		esp_spp_write(s_conn.handle, strlen(message), (uint8_t *)message);
	}
}

void bt_startup(void) {
	char bda_str[18] = {0};
	/* Setup Queue for BT incoming Data */
	btDataQueue = xQueueCreate(10, sizeof(btData_t));

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
		ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
		ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s", __func__,
				 esp_err_to_name(ret));
		return;
	}

	if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
		ESP_LOGE(SPP_TAG, "%s enable controller failed: %s", __func__,
				 esp_err_to_name(ret));
		return;
	}

	esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
#if (CONFIG_EXAMPLE_SSP_ENABLED == false)
	bluedroid_cfg.ssp_en = false;
#endif
	if ((ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK) {
		ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s", __func__,
				 esp_err_to_name(ret));
		return;
	}

	if ((ret = esp_bluedroid_enable()) != ESP_OK) {
		ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s", __func__,
				 esp_err_to_name(ret));
		return;
	}

	if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
		ESP_LOGE(SPP_TAG, "%s gap register failed: %s", __func__,
				 esp_err_to_name(ret));
		return;
	}

	if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
		ESP_LOGE(SPP_TAG, "%s spp register failed: %s", __func__,
				 esp_err_to_name(ret));
		return;
	}

	esp_spp_cfg_t bt_spp_cfg = {
		.mode = esp_spp_mode,
		.enable_l2cap_ertm = esp_spp_enable_l2cap_ertm,
		.tx_buffer_size = 0, /* Only used for ESP_SPP_MODE_VFS mode */
	};
	if ((ret = esp_spp_enhanced_init(&bt_spp_cfg)) != ESP_OK) {
		ESP_LOGE(SPP_TAG, "%s spp init failed: %s", __func__,
				 esp_err_to_name(ret));
		return;
	}

	/* Set default parameters for Secure Simple Pairing */
	esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
	esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
	esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

	/*
	 * Set default parameters for Legacy Pairing
	 * Use variable pin, input pin code when pairing
	 */
	esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
	esp_bt_pin_code_t pin_code;
	esp_bt_gap_set_pin(pin_type, 0, pin_code);

	ESP_LOGI(
		SPP_TAG, "Own address:[%s]",
		bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
}
