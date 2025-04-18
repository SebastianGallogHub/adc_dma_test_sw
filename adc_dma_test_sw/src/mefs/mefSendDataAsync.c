/*
 * mefSendDataAsync.c
 *
 *  Created on: Apr 18, 2025
 *      Author: sebas
 */


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
u64 sector_rd_buffer[SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN)] __attribute__ ((aligned (64)));

/****************************************************************************/

void mefSendDataAsync(){
	static MEF_SEND_DATA_STATE st = WAITING_FOR_DATA_TO_SEND;

	switch (st){
	case WAITING_FOR_DATA_TO_SEND:
		if(SD_SectorsToRead() > 1){
			st = SENDING_DATA;
		}

		break;

	case SENDING_DATA:
		if(SD_SectorsToRead() > 0) {
			if(UART_DoneSendBuffer()){
				SD_ReadNextSector((unsigned char*)sector_rd_buffer);
				UART_SendBufferAsync((u32)sector_rd_buffer, SD_SECTOR_SIZE, AXITAR_AXIDMA_TRANSFER_LEN);
			}
		}

		if(SD_SectorsToRead() <= 0){
			st = AWAITING_LAST_DATA_SEND_DONE;
		}

		break;

	case AWAITING_LAST_DATA_SEND_DONE:
		if(UART_DoneSendBuffer()){
			st = WAITING_FOR_DATA_TO_SEND;
		}

		break;

	default:
		st = WAITING_FOR_DATA_TO_SEND;
		break;
	}
}
