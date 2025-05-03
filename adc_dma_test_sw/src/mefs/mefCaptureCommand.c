/*
 * mefCaptureCommand.c
 *
 *  Created on: Apr 18, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../mefs/mefCaptureCommand.h"

#include "../includes/log.h"
#include "../AXITAR/axitar.h"
#include "../SD_CARD/sd_card.h"
#include "../UART/uart_mefCommand.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
u32 p;
UART_COMMAND c;

/****************************************************************************/

void mefCaptureCommand_Init(){

}

void mefCaptureCommand(){
	c = UART_GetCommand();

	if(c == CMD_START){
		AXITAR_Start();
	}

	if(c == CMD_STOP){
		AXITAR_Stop();
	}

	if(c == CMD_CH0_H ||
	   c == CMD_CH1_H){

		while(1){
			if(UART_HasParameter()){
				p = UART_GetParameter();
				break;
			}
		}

		if(c == CMD_CH0_H){
			AXITAR_SetHysteresis(0, p);
			LOG(2, "Canal 0, histéresis (%u ; %u)", AXITAR_LowHist(p), AXITAR_HighHist(p));
		}

		if(c == CMD_CH1_H){
			AXITAR_SetHysteresis(1, p);
			LOG(2, "Canal 1, histéresis (%u ; %u)", AXITAR_LowHist(p), AXITAR_HighHist(p));
		}
	}
}
