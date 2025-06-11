/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : mefSendDataAsync.c
 * Descripción         : Archivo de implementación de la mef que gestiona
 * 						 la lógica del envío contínuo y asincrónico
 * 						 de datos a PC mediante UART.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 18/04/2025
 * Fecha de modificación: 11/06/2025
 * Versión             : v1.0
 *
 * Institución         : Universidad Nacional de Rosario (UNR)
 * Carrera             : Ingeniería Electrónica
 *
 * Derechos reservados:
 * Este código ha sido desarrollado en el marco del Proyecto Final de Ingeniería
 * por Sebastián Nahuel Gallo. Su uso está autorizado únicamente por la
 * Comisión Nacional de Energía Atómica (CNEA) con fines internos.
 * Queda prohibida su reproducción, modificación o distribución sin
 * autorización expresa por escrito del autor.
 ***************************************************************/

/***************************** Include Files *******************************/
#include "../mefs/mefSendDataAsync.h"

#include "../UART/uart.h"
#include "../AXITAR/axitar.h"
#include "../SD_CARD/sd_card.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/
typedef enum{
	WAITING_FOR_DATA_TO_SEND,
	SENDING_DATA,
	AWAITING_LAST_DATA_SEND_DONE,
} MEF_SEND_DATA_STATE;

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

MEF_SEND_DATA_STATE st = WAITING_FOR_DATA_TO_SEND;

u8 cancelAsync = 0;

u64 sector_rd_buffer[SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN)] __attribute__ ((aligned (64)));

/****************************************************************************/

void mefSendDataAsync_Init(){
	SD_Init();
}

void mefSendDataAsync_Reset(){
	SD_ResetRB();

	cancelAsync = 0;

	st = WAITING_FOR_DATA_TO_SEND;
}

void mefSendDataAsync_Cancel(){
	UART_SendBufferAsync_Cancel();

	cancelAsync = 1;
}

int mefSendDataAsync(){
	int res = 0;

	switch (st){
		case WAITING_FOR_DATA_TO_SEND:
			if(SD_GetSectorsToRead() > 1 && !cancelAsync){
				st = SENDING_DATA;
			} else {
				res = 1;
			}
			break;

		case SENDING_DATA:
			if(SD_GetSectorsToRead() > 0) {
				if(UART_DoneSendBuffer()){
					SD_ReadNextSector((unsigned char*)sector_rd_buffer);
					UART_SendBufferAsync((u32)sector_rd_buffer, SD_SECTOR_SIZE, AXITAR_AXIDMA_TRANSFER_LEN, cancelAsync);
				}
			}

			if(SD_GetSectorsToRead() <= 0){
				st = AWAITING_LAST_DATA_SEND_DONE;
			}

			if(cancelAsync){
				st = WAITING_FOR_DATA_TO_SEND;
			}

			break;

		case AWAITING_LAST_DATA_SEND_DONE:
			if(UART_DoneSendBuffer()){
				res = 1;
				cancelAsync = 0;
				st = WAITING_FOR_DATA_TO_SEND;
			}

			break;

		default:
			st = WAITING_FOR_DATA_TO_SEND;
			break;
	}

	return res;
}
