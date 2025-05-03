/*
 * mefTAR.C
 *
 *  Created on: May 3, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../mefs/mefTAR.h"

#include "../includes/log.h"
#include "../includes/assert.h"
#include "../UART/uart.h"
#include "../UART/uart_mefCommand.h"
#include "../mefs/mefSendDataAsync.h"
#include "../AXITAR/axitar.h"
#include "../InterruptSystem/interruptSystem.h"


/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/
typedef enum{
	INIT_MODULES,
	IDLE,
	SET_CH_HYSTERESIS,
	SEND_CONFIG_LOG,
	START_TAR,
	SENDING_DATA_ASYNC,
	STOP_TAR,
	AWAITING_LAST_DATA_SEND,
} MEF_TAR_STATE;

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

MEF_TAR_STATE state;
UART_COMMAND cmd = CMD_NONE;
u32 param = 0;

/****************************************************************************/

void mefTAR_Init(){
	// Como manipula el puerto serie hay que inicializarlo
	// antes de empezar a mandar al terminal
	UART_Init();

	AXITAR_Init();

	mefSendDataAsync_Init();

	state = INIT_MODULES;
}

int mefTAR(){
	switch(state){
		case INIT_MODULES:
			// Se configura el sistema de interrupciones con los handlers
			// registrados hasta este punto
			ASSERT_SUCCESS(
					IntrSystem_Setup(),
					"Fallo al inicializar el sistema de interrupciones");

			// Se inicializa la recepción siempre después de configurar
			// el sistema de interrupciones
			UART_SetupRx();

			state = IDLE;
			break;

		case IDLE:
			cmd = UART_GetCommand();

			if(cmd == CMD_CH0_H || cmd == CMD_CH1_H){
				state = SET_CH_HYSTERESIS;
			}

			if(cmd == CMD_GET_CONF){
				state = SEND_CONFIG_LOG;
			}

			if(cmd == CMD_START){
				state = START_TAR;
			}
			break;

		case SET_CH_HYSTERESIS:
			if(UART_HasParameter()){
				param = UART_GetParameter();

				if(cmd == CMD_CH0_H){
					AXITAR_SetHysteresis(0, param);
				}

				if(cmd == CMD_CH1_H){
					AXITAR_SetHysteresis(1, param);
				}

				state = IDLE;
			}
			break;

		case SEND_CONFIG_LOG:
			LOG(0, "%%");
			AXITAR_PrintConfigLog(0);
			LOG(0, "%%");

			state = IDLE;
			break;

		case START_TAR:
			mefSendDataAsync_Reset();
			AXITAR_Start();

			state = SENDING_DATA_ASYNC;
			break;

		case SENDING_DATA_ASYNC:
			cmd = UART_GetCommand();
			mefSendDataAsync();

			if (cmd == CMD_STOP){
				state = STOP_TAR;
			}
			break;

		case STOP_TAR:
			AXITAR_Stop();
			mefSendDataAsync_CancelAsync();
			state = AWAITING_LAST_DATA_SEND;
			break;

		case AWAITING_LAST_DATA_SEND:
			if(mefSendDataAsync()){
				state = IDLE;
			}
			break;

		default:
			state = IDLE;
			break;
	}
	return 0;
}

