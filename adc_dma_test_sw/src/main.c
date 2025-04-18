
/***************************** Include Files *******************************/
#define MAIN
#ifdef MAIN

#include <unistd.h>

#include "xparameters.h"
#include "xil_printf.h"

#include "mefSendDataAsync.h"
#include "includes/log.h"
#include "includes/assert.h"
#include "AXITAR/axitar.h"
#include "AXITAR/axitar_axidma.h"
#include "SD_CARD/sd_card.h"
#include "UART/uart.h"
#include "UART/uart_mefCommand.h"

#include "ZMOD_ADC1410/zmodadc1410.h"
#include "InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

/****************************************************************************/

int main(){
	u32 p;
	UART_COMMAND c;
	u16 h0_low = 0, h0_high = 0;
	u16 h1_low = 0, h1_high = 0;

	UART_Init();

	LOG(0, "--------------------- INICIO MAIN -----------------------");
	LOG(1, "PRUEBA LÓGICA DE DETECCIÓN DE PULSO + RINGBUFFER SD");
	usleep(2000);

	SD_Init();
	AXIDMA_Init();
	AXITAR_Init();
	ZMODADC1410_Init();

	ASSERT_SUCCESS(
			IntrSystem_Setup(),
			"Fallo al inicializar el sistema de interrupciones");

	UART_SetupRx();
	AXIDMA_SetupRx(
		SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN) * 2,  	// Cantidad de DATOS a recibir en el buffer
		AXITAR_AXIDMA_TRANSFER_LEN,								// Longitud de 1 dato en bytes
		SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN),		// Coalescencia
		(AXIDMA_ProcessBufferDelegate)SD_WriteNextSector);		// Handler para procesar el buffer

//	AXITAR_SetHysteresis(0, (((u32)1000/ZMODADC1410_RESOLUTION)<<16)|(3000/ZMODADC1410_RESOLUTION));
//	AXITAR_SetHysteresis(1, (((u32)1000/ZMODADC1410_RESOLUTION)<<16)|(3000/ZMODADC1410_RESOLUTION));

	while(1){

		mefSendDataAsync();

		c = UART_GetCommand();

		if(c == CMD_START){
			LOG(1, "----- Inicio adquisición -----");
			SD_ResetRB();
			usleep(2000);
			AXITAR_Start();
		}

		if(c == CMD_STOP){
			AXITAR_StopAll();
			AXIDMA_Reset();
		}

		if(c == CMD_CH0_H ||
		   c == CMD_CH1_H){

			while(1){
				if(UART_HasParameter()){
					p = UART_GetParameter();
					break;
				}
			}

			switch (c) {
				case CMD_CH0_H: AXITAR_SetHysteresis(0, p); break;
				case CMD_CH1_H: AXITAR_SetHysteresis(1, p); break;
				default: break;
			}

			if(c == CMD_CH0_H){
				LOG(2, "Canal 0, histéresis (%u ; %u)", (u16)(p >> 16), (u16)(p & 0xFFFF));
			}

			if(c == CMD_CH1_H){
				LOG(2, "Canal 1, histéresis (%u ; %u)", (u16)(p >> 16), (u16)(p & 0xFFFF));
			}
		}
	}
	return 0;
}

#endif // MAIN



















