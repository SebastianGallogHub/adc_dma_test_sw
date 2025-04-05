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

	AXI_DMA_Init(TAR_DMA_TRANSFER_LEN, (AXI_DMA_ProcessBufferDelegate)UARTPS_0_SendBufferAsync);

	ZMODADC1410_Init();

	TAR_Init(TAR_TRANSFER_PERIOD);

	SetupIntrSystem();

	UARTPS_0_StartRx();

	AXI_DMA_SetupRx(AXI_DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN);

	LOG(1, "Enviando %d bytes", AXI_DMA_NUMBER_OF_TRANSFERS * TAR_DMA_TRANSFER_LEN);

	LOG(0, "----- Inicio interrupciones -----");

	xil_printf("&");

	TAR_Start_master_test();

	// Espero hasta que se den las transacciones
	while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS-3);

	TAR_StopAll();

	AXI_DMA_Reset();

	while(1){
		if(UARTPS_0_DoneSendBuffer()){
			xil_printf("&");
			break;
		}
	}

	PrintRxData();

//	xil_printf("&");

//	UARTPS_0_SendBufferAsync((u32)AXI_DMA_RX_BUFFER_BASE, AXI_DMA_NUMBER_OF_TRANSFERS * TAR_DMA_TRANSFER_LEN, TAR_DMA_TRANSFER_LEN);

//	while(1){
//		if(UARTPS_0_DoneTx()){
//			if(DMAPS_Done()){
//				xil_printf("&");
//				break;
//			}
//		}
//	}

	usleep(1000);
	LOG(0,"\nSe ejecutó correctamente el ejemplo");

	TAR_Start_master_test();
}

void PrintRxData()
{
	u32* buffer  = (u32*)AXI_DMA_RX_BUFFER_BASE;

	LOG(0, "-----------------------------------------------------------------------------------------------------------------");
	LOG(1, "Datos recibidos");
	LOG(3, "CH1 \t|\tCH2", axiTarTransferCount);
	for (u32 i = 0; i<= axiDmaTransferCount; i+=1){
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
