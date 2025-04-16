/*
 * axitar_xuartps.c
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
//#define MAIN_axitar_live_sd
#ifdef MAIN_axitar_live_sd

#include <stdio.h>

#include "xuartps.h"
#include "xil_printf.h"
#include "sleep.h"

#include "includes/log.h"
#include "AXI_TAR/axitar.h"
#include "AXI_TAR/axitar_axidma.h"
#include "SD_CARD/sd_card.h"
#include "UART_DMA/xuartps_0.h"
#include "ZMOD_ADC1410/zmodadc1410.h"
#include "InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

#define WORDS_PER_SECTOR SD_WORDS_PER_SECTOR(sizeof(u32))

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void PrintRxData();

/************************** Variable Definitions ***************************/

u32 sector_rd_buffer[WORDS_PER_SECTOR] __attribute__ ((aligned (32)));

/****************************************************************************/

int mefDatos(){
	static int st = 0;
	int res = 0;

	switch (st){
	case 0:
		if(SD_SectorsToRead() > 1){
			xil_printf("&");
			st = 1;
		}

		break;

	case 1:
		if(SD_SectorsToRead() > 0) {
			if(UARTPS_0_DoneSendBuffer()){
				SD_ReadNextSector((unsigned char*)sector_rd_buffer);
				UARTPS_0_SendBufferAsync((u32)sector_rd_buffer, SD_SECTOR_SIZE, sizeof(u32));
			}
		}

		if(SD_SectorsToRead() <= 0){
			st = 2;
		}

		break;

	case 2:
		if(UARTPS_0_DoneSendBuffer()){
			xil_printf("&");
			st = 0;
			res = 1;
		}

		break;

	default:
		st = 0;
		break;
	}

	return res;
}


int main2(){
	UARTPS_0_Init();
	SD_Init();
	AXI_DMA_Init();
	ZMODADC1410_Init();
	AXI_TAR_master_test_Init(AXI_TAR_TRANSFER_PERIOD);

	SetupIntrSystem();

	UARTPS_0_StartRx();

	AXI_DMA_SetupRx(
			SD_WORDS_PER_SECTOR(AXI_TAR_DMA_TRANSFER_LEN)*2,  		// Cantidad de DATOS a recibir en el buffer
			AXI_TAR_DMA_TRANSFER_LEN,								// Longitud de 1 dato en bytes
			SD_WORDS_PER_SECTOR(AXI_TAR_DMA_TRANSFER_LEN),			// Coalescencia
			(AXI_DMA_ProcessBufferDelegate)SD_WriteNextSector);		// Handler para procesar el buffer

	LOG(0, "----- Inicio interrupciones -----");

	AXI_TAR_Start_master_test();

	while(1){
		// mefMedicion
		if (axiDmaTransferCount >= SD_WORDS_PER_SECTOR(AXI_TAR_DMA_TRANSFER_LEN) * 10){
			AXI_TAR_StopAll();
			AXI_DMA_Reset();
		}

		if (mefDatos()){
			break;
		}
	}

	usleep(1000);
	LOG(0,"\nSe ejecutó correctamente el ejemplo");
}

void PrintRxData()
{
	u32* buffer  = (u32*)AXI_DMA_RX_BUFFER_BASE;

	LOG(0, "-----------------------------------------------------------------------------------------------------------------");
	LOG(1, "Datos recibidos");
	LOG(3, "CH1 \t|\tCH2");
	for (u32 i = 0; i<= SD_WORDS_PER_SECTOR(AXI_TAR_DMA_TRANSFER_LEN); i++){
		LOG(2, "%d)\t;%d  \t;\t%d;\t\t[0x%08x]", i,  (u16)((buffer[i] >> 16) & 0xffff), (u16)(buffer[i] & 0xffFF), &buffer[i]);
	}
	LOG(0, "------------------------------------------------------------------------------------------------------------------");
	LOG(2, "Interrupciones recibidas por DMA: %d", axiDmaIntCount);
	LOG(2, "Transferencias recibidas por DMA: %d", axiDmaTransferCount);
	LOG(2, "Transferencias lanzadas por TAR: %d", axiTarTransferCount);
	LOG(0, "------------------------------------------------------------------------------------------------------------------");

	return;
}
#endif
