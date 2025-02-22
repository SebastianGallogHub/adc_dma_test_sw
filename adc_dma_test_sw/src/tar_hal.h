/*
 * tar_hal.h
 *
 *  Created on: Feb 12, 2025
 *      Author: sebas
 */

#ifndef SRC_TAR_HAL_H_
#define SRC_TAR_HAL_H_

#include "xil_types.h"

typedef enum TAR_SERIAL_COMMANDS_ENUM
{
	TAR_NONE = 0,
	TAR_START,
	TAR_A_H_LOW,
	TAR_A_H_HIGH,
	TAR_B_H_LOW,
	TAR_B_H_HIGH,
}tar_command;

typedef enum TAR_CHANNEL_ENUM
{
	TAR_CH_A = 0x01,
	TAR_CH_B = 0x10,
}tar_channel;

// Todas las funciones que devuelvan int es porque devuelven un código de error.
// 0 = no error;

void TAR_InitAll(void);

// A esta función hay que hacerle polling
tar_command TAR_GetCommand(u16 * parameter);

int TAR_SetHysteresis(tar_channel channel, u16 high, u16 low);
int TAR_Start();
// Reinicia los contadores internos del IP y los buffer de PS, no las histéresis
int TAR_Stop();

int TAR_Timer_SetHandler(void * timerHandler);
int TAR_Timer_Start(u64 period_ms);
void TAR_Timer_Stop(void);

#endif /* SRC_TAR_HAL_H_ */
