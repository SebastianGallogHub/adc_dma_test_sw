/*
 * tar_hal.h
 *
 *  Created on: Feb 12, 2025
 *      Author: sebas
 */

#ifndef SRC_TAR_HAL_H_
#define SRC_TAR_HAL_H_

#include "xil_types.h"

typedef enum
{
	NONE = 0,
	START,
	CH1_H_LOW,
	CH1_H_HIGH,
	CH2_H_LOW,
	CH2_H_HIGH,
}tar_command;

typedef enum
{
	LOW,HIGH,
}tar_histeresys_level;

typedef enum
{
	CH1 = 0x01,
	CH2 = 0x10,
}tar_channel;

// Todas las funciones que devuelvan int es porque devuelven un código de error.
// 0 = no error;

void TAR_InitAll();

// A esta función hay que hacerle pollingo
tar_command TAR_GetCommand(u16 * parameter);

int TAR_SetHysteresis(tar_channel channel, tar_histeresys_level level, u16 value); //0: low; 1: high
int TAR_Start(u32 period_ms); // Max 1000 horas
int TAR_Stop();
int TAR_State(); // 0 parado; 1 funcionando; 2 error

#endif /* SRC_TAR_HAL_H_ */
