/*
 * uart_mefCommand.c
 *
 *  Created on: Apr 18, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../UART/uart_mefCommand.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/
typedef enum{
	WAITING_COMMAND,
	COMMAND_RECEIVED,
	WAITING_PARAMETER,
	WAITING_PARAMETER_LSB,
}MEF_COMMAND_STATE;

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
volatile u8 command = 0;
volatile u8 parameter_f = 0;
volatile u8 parameter_count = 0;
volatile u32 parameter = 0;

/****************************************************************************/

UART_COMMAND UART_GetCommand(){
	UART_COMMAND c = (UART_COMMAND)command;
	command = 0;
	return c;
}

u8 UART_HasParameter(){
	return parameter_f;
}

u32 UART_GetParameter(){
	u32 p = parameter;
	parameter = 0;
	parameter_f = 0;
	return p;
}

void UART_mefCommand(u8 chr){
	static MEF_COMMAND_STATE state = WAITING_COMMAND;

	switch (state) {
	case WAITING_COMMAND:
		if(chr == CMD_HEADER)
			state = COMMAND_RECEIVED;
		break;

	case COMMAND_RECEIVED:
		if(chr == CMD_START ||
		   chr == CMD_GET_CONF){
			command = chr;
			state = WAITING_COMMAND;
		}

		if(chr == CMD_STOP){
			command = chr;
			state = WAITING_COMMAND;
		}

		if(chr == CMD_CH0_H ||
		   chr == CMD_CH1_H ){
			command = chr;
			parameter = 0;
			parameter_f = 0;
			parameter_count = 0;
			state = WAITING_PARAMETER;
		}

		break;

	case WAITING_PARAMETER:
		parameter = (parameter << 8) | chr;
		parameter_count ++;

		if(parameter_count > 3)
		{
			parameter_f = 1;
			state = WAITING_COMMAND;
		}
		break;

	default:
		state = WAITING_COMMAND;
		break;
	}
}
