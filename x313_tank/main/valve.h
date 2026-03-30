/*
 * valve.h
 *
 *  Created on: 25.03.2026
 *      Author: al
 */

#ifndef MAIN_VALVE_H_
#define MAIN_VALVE_H_
#include <stdbool.h>

// 4% -> 14 Liter, damit er nicht zu oft auffüllt
#define VENTIL_HYSTERESE 40 
// abschalten Handmodus wenn nachgefüllt wird
#define VENTIL_HYSTERESE_HAND 15 
#define MAX_WASSERSTAND_PROMILLE 965
#define MAX_WASSERSTAND_ABSOLUT 1000

typedef enum {
	vAllesOk,
	vZuVielAnfang,
	vZuVielWarten,
	vWiederOk,
	vHandEin,
	vHandEinWarten,
	vHandUeberfuellt,
	vAutoEin,
} valveStatus_t;


void valveTask(void *parameters);
void setVentil(bool onOff);

void setVentilHand(bool onOff);
bool getVentilHand(void);
bool getVentil(void);
valveStatus_t getVentilStatus (void); 

#endif /* MAIN_VALVE_H_ */
