/*
 * axitar_xuartps.c
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/

#include "xuartps.h"

#include "xil_printf.h"

#include "axitar.h"
#include "axitar_axidma.h"
#include "xuartps_0.h"
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

#define MAIN_axitar
#ifdef MAIN_axitar
int main(){
	UARTPS_0_Init();

	AXI_DMA_Init();

	ZMODADC1410_Init();

	TAR_Init(TAR_TRANSFER_PERIOD);

	SetupIntrSystem();

	AXI_DMA_RxInit(AXI_DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN);

	LOG(0, "----- Inicio interrupciones -----");
	TAR_Start_master_test();

	// Espero hasta que se den las transacciones
	while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS){}

	TAR_StopAll();

	AXI_DMA_Reset();


	LOG(0, "Interrupciones recibidas por DMA: %d", axiDmaIntCount);
	LOG(0, "Transferencias recibidas por DMA: %d", axiDmaTransferCount);
	LOG(0, "Transferencias lanzadas por TAR: %d", axiTarTransferCount);

//	UARTPS_0_ConfigSendAsync((u32)AXI_DMA_RX_BUFFER_BASE, axiDmaTransferCount * TAR_DMA_TRANSFER_LEN);
	UARTPS_0_ConfigSendAsync((u32)AXI_DMA_RX_BUFFER_BASE, 64);

	UARTPS_0_SendAsync();

	while(1);

	LOG(0,"\nSe ejecuto correctamente el ejemplo");
}

void PrintRxData()
{
	u32* buffer  = (u32*)AXI_DMA_RX_BUFFER_BASE;
	LOG_LINE;LOG_LINE;
//	LOG(1, "Datos recibidos (%d ms)", GetElapsed_ms);
	LOG(1, "Datos recibidos\n");

	LOG(2, "--------------------------------------");
	LOG(2, "\t\t\tCH1 \t|\tCH2", axiTarTransferCount);
	for (u32 i = 0; i<= axiDmaTransferCount; i+=1){
		LOG(2, "%d)\t%d  \t|\t%d\t\t[0x%08x]", i,  (u16)((buffer[i] >> 16) & 0xffff), (u16)(buffer[i] & 0xffFF), &buffer[i]);
	}
	LOG(2, "--------------------------------------");
	LOG(2, "Interrupciones recibidas por DMA: %d", axiDmaIntCount);
	LOG(2, "Transferencias recibidas por DMA: %d", axiDmaTransferCount);
	LOG(2, "Transferencias lanzadas por TAR: %d", axiTarTransferCount);

	return;
}
#endif
