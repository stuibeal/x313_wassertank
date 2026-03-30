/*
 * pumpe.h
 *
 *  Created on: 25.03.2026
 *      Author: al
 */

#ifndef MAIN_PUMPE_H_
#define MAIN_PUMPE_H_
#include <stdbool.h>

#define WARN_WASSERSTAND_PROMILLE 100
#define AUS_WASSERSTAND_PROMILLE 20
#define PUMPEN_HYSTERESE 25
#define PUMPEN_TAG "PUMPE"

typedef enum {
	beginnAus,
	anschalten,
	normalAn,
	warnen,
	warnenWarten,
	zuWenigAnfang,
	zuWenigWarten,
	handEin,
	handWarten,
} pumpenStatus_t;

void pumpeTask(void *arg);
void setPumpe(bool onOff);

bool getPumpeStatus(void);
bool getPumpeHand(void);
void setPumpeHand(bool onOff);

#endif /* MAIN_PUMPE_H_ */
