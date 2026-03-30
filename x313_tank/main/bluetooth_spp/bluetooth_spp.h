/*
* Bluetooth stack for X313 Tank
*
*
*/
#ifndef BLUETOOTH_SPP_H_

#define BLUETOOTH_SPP_H_
#define SPP_TAG "X313_Tank"
#define SPP_SERVER_NAME "X313_SPP_SERVER"
#define SPP_SHOW_DATA 1
#define SPP_SHOW_SPEED 1
#define SPP_SHOW_MODE SPP_SHOW_DATA    /*Choose show mode: show data or speed*/

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "time.h"
#include "sys/time.h"
//#include "global.h"

static const char local_device_name[] = "X313_TANK";
static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const bool esp_spp_enable_l2cap_ertm = true;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;
static const bool bt_connection = 64; // nur für die Lesbarkeit, Zahl egal

//static char *bda2str(uint8_t * bda, char *str, size_t size);

typedef enum {
	pumpe = 0x50,
	ventil = 0x56,
	helligkeit = 0x48,
	beleuchtung = 0x4C
} btBefehl;

// queues
extern QueueHandle_t btDataQueue;


typedef struct {
    uint32_t handle;
    bool connected;
    bool congested;
} spp_conn_t;

typedef struct {
	uint32_t firstByte;
	char rohdaten[10];
	uint32_t len;
	int data; 
} btData_t;

//static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void sendBTMessage(char *message);
void bt_startup(void);
#endif