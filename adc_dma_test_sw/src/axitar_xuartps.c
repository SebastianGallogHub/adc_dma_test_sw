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

int main(){
	int i ;

	// todo agregar parámetros  para saber de donde sacar los datos
	UARTPS_0_Init();

	// Para que coexistan ejemplos ---------------------------------------------------------

	AXI_DMA_Init();

	ZMODADC1410_Init();

	TAR_Init(TAR_TRANSFER_COUNT);

//	i ++;
//	LOG(0, "--------------------- (%d) -----------------------", i);
	ASSERT_SUCCESS(
			SetupIntrSystem(),
			"Fallo al inicializar el sistema de interrupciones");

	ASSERT_SUCCESS(
			AXI_DMA_RxInit(AXI_DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN),
			"Fallo al inicializar el DMA.");

	LOG(1, "----- Inicio interrupciones");
	TAR_Start_master_test();
//	XTime_GetTime(&tStart);

//	Status = Xil_WaitForEventSet(1000000U, 1, &Error);
//	if (Status == XST_SUCCESS) {
//		LOG(0, "--> Receive error %d", Status);
//		goto goOut;
//	}

	// Espero hasta que se den las transacciones
	while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS){}

//	DisableIntrSystem();

//	XTime_GetTime(&tFinish);

	TAR_StopAll();

	AXI_DMA_Reset();

	//Imprimir todos los valores recibidos junto a su dirección
	PrintRxData();
	axiDmaIntCount = 0;
	axiDmaTransferCount = 0;
	axiTarTransferCount = 0;

	// Para que coexistan ejemplos ---------------------------------------------------------

	xil_printf("\nComandos:\t-'a' : Enviar datos en contínuo\n\t-'f' : Detener envío de datos\n");

	UARTPS_0_Test();
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

