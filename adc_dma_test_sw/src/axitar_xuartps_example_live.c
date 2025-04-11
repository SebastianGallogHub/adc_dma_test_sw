/*
 * axitar_xuartps.c
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include <stdio.h>

#include "xuartps.h"

#include "xil_printf.h"

#include "sleep.h"
#include "axitar.h"
#include "axitar_axidma.h"
#include "xuartps_0.h"
#include "xuartps_0_xdmaps.h"
#include "zmodadc1410.h"
#include "interruptSystem.h"

#include "log.h"
#include "assert.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void PrintRxData();

/************************** Variable Definitions ***************************/

/****************************************************************************/

#define MAIN_axitar_live
#ifdef MAIN_axitar_live
int main(){

	UARTPS_0_Init();
	AXI_DMA_Init();
	ZMODADC1410_Init();
	AXI_TAR_master_test_Init(AXI_TAR_TRANSFER_PERIOD);

	SetupIntrSystem();

	UARTPS_0_StartRx();

	AXI_DMA_SetupRx(
			AXI_DMA_NUMBER_OF_TRANSFERS,
			AXI_TAR_DMA_TRANSFER_LEN,
			AXI_DMA_NUMBER_OF_TRANSFERS,//(UART_TX_FIFO_DEPTH - AXI_TAR_DMA_TRANSFER_LEN)/AXI_TAR_DMA_TRANSFER_LEN, // máxima cantidad de DATOS que soporta la UART
			(AXI_DMA_ProcessBufferDelegate)UARTPS_0_SendBufferAsync);

//	LOG(1, "Enviando %d bytes (%d datos)",
//			AXI_DMA_NUMBER_OF_TRANSFERS * AXI_TAR_DMA_TRANSFER_LEN,
//			(UART_TX_FIFO_DEPTH - AXI_TAR_DMA_TRANSFER_LEN)/AXI_TAR_DMA_TRANSFER_LEN);

	LOG(0, "----- Inicio interrupciones -----");

	xil_printf("&");

	AXI_TAR_Start_master_test();

	// Espero hasta que se den las transacciones
	while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS * 2);

	AXI_TAR_StopAll();

	AXI_DMA_Reset();

	while(1){
		if(UARTPS_0_DoneSendBuffer()){
			xil_printf("&");
			break;
		}
	}

//	PrintRxData();

	usleep(1000);
	LOG(0,"\nSe ejecutó correctamente el ejemplo");
}

void PrintRxData()
{
	u32* buffer  = (u32*)AXI_DMA_RX_BUFFER_BASE;

	LOG(0, "-----------------------------------------------------------------------------------------------------------------");
	LOG(1, "Datos recibidos");
	LOG(3, "CH1 \t|\tCH2");
	for (u32 i = 0; i<= AXI_DMA_NUMBER_OF_TRANSFERS; i+=1){
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
