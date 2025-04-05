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

	TAR_Init(TAR_TRANSFER_PERIOD);

	SetupIntrSystem();

	TAR_Start_master_test();

	UARTPS_0_StartRx();

	AXI_DMA_SetupRx(AXI_DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN);

	LOG(0, "----- Inicio interrupciones -----");
	TAR_Start_master_test();

	// Espero hasta que se den las transacciones
	while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS);

	TAR_StopAll();

	AXI_DMA_Reset();

	PrintRxData();

	// Cargar un buffer con los datos de AXI_DMA_RX_BUFFER formateados para ver en pantalla
//	int maxCntDATOS_EnviarPorTrans = 15;
//	u32 maxCntBYTES_EnviarPorTrans = TAR_DMA_TRANSFER_LEN * maxCntDATOS_EnviarPorTrans;
//	u32 cntBYTES_RestaEnviar = AXI_DMA_NUMBER_OF_TRANSFERS * TAR_DMA_TRANSFER_LEN; // Cuantos BYTES me quedan por enviar
//	u32 cntBYTES_EnviarAhora = 0; // Desde donde respecto de la base
//	u32 cntBYTES_Enviados = 0;
//	u32 *nextBuffer = (u32*)AXI_DMA_RX_BUFFER_BASE;
//
//	LOG(1,"Enviando %d bytes de a %d bytes (%d pares) desde [0x%08x] con delays de max %dus",
//			cntBYTES_RestaEnviar, maxCntBYTES_EnviarPorTrans,
//			maxCntBYTES_EnviarPorTrans/TAR_DMA_TRANSFER_LEN,
//			nextBuffer, UART_MIN_DELAY_PER_BYTE_SEND*maxCntDATOS_EnviarPorTrans);

//	xil_printf("&");
//	while (cntBYTES_RestaEnviar) { //Continúo siempre que haya datos para enviar
//		if(UARTPS_0_DoneTx())	{
//			if(DMAPS_Done()){
//				// Calculo la cantidad a enviar en este lote según maxCntBYTES_EnviarPorTrans
//				cntBYTES_EnviarAhora =  cntBYTES_RestaEnviar > maxCntBYTES_EnviarPorTrans? maxCntBYTES_EnviarPorTrans: cntBYTES_RestaEnviar;
//
//				// Envío
//				UARTPS_0_SendAsync((u32)nextBuffer, cntBYTES_EnviarAhora);
//
//				// Registro lo enviado
//				nextBuffer += cntBYTES_EnviarAhora/TAR_DMA_TRANSFER_LEN; // CANTIDAD EN DATOS NO EN BYTES!!!!
//				cntBYTES_Enviados += cntBYTES_EnviarAhora;
//				cntBYTES_RestaEnviar -= cntBYTES_EnviarAhora;
//			}
//		}
//	}

	LOG(1, "Enviando %d bytes", AXI_DMA_NUMBER_OF_TRANSFERS * TAR_DMA_TRANSFER_LEN);

	xil_printf("&");

	UARTPS_0_SendBufferAsync((u32)AXI_DMA_RX_BUFFER_BASE, AXI_DMA_NUMBER_OF_TRANSFERS * TAR_DMA_TRANSFER_LEN, TAR_DMA_TRANSFER_LEN);

	while(1){
		if(UARTPS_0_DoneTx()){
			if(DMAPS_Done()){
				xil_printf("&");
				break;
			}
		}
	}

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
