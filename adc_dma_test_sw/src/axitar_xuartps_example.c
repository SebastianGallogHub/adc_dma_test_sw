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

#define MAIN_axitar
#ifdef MAIN_axitar
int main(){
	UARTPS_0_Init();

	AXI_DMA_Init();

	ZMODADC1410_Init();

	TAR_Init(TAR_TRANSFER_PERIOD);

	SetupIntrSystem();

	TAR_Start_master_test();

//	while(1);

	UARTPS_0_StartRx();

	AXI_DMA_SetupRx(AXI_DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN);

	LOG(0, "----- Inicio interrupciones -----");
	TAR_Start_master_test();

	// Espero hasta que se den las transacciones
	while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS);

	TAR_StopAll();

	AXI_DMA_Reset();

//	LOG(0, "Interrupciones recibidas por DMA: %d", axiDmaIntCount);
//	LOG(0, "Transferencias recibidas por DMA: %d", axiDmaTransferCount);
//	LOG(0, "Transferencias lanzadas por TAR: %d", axiTarTransferCount);

	PrintRxData();

	// Cargar un buffer con los datos de AXI_DMA_RX_BUFFER formateados para ver en pantalla
	u32 maxCntAEnviarPorTrans = 8; // Mando datos de a 40 Bytes, son 10 valores dobles de 16b
	u32 cntRestaEnviar = 0;
	u32 cntEnviados = 0;
	u32 cntEnviarAhora = 0;
	u32 *nextBuffer = (u32*)AXI_DMA_RX_BUFFER_BASE;

	// Enviar todos los datos de a 64 bytes (máximo de uart tx)
	cntRestaEnviar = AXI_DMA_NUMBER_OF_TRANSFERS*sizeof(u32); // Cuantos me queda por enviar
	cntEnviarAhora = 0; // Desde donde respecto de la base
	xil_printf("&");

	while (cntRestaEnviar) { //Continúo siempre que haya datos para enviar

		if(uart0DoneTx)
		{
			if(DMAPS_Done())
			{
				uart0DoneTx = 0;

				// Calculo la cantidad a enviar en este lote según maxCntAEnviarPorTrans
				cntEnviarAhora =  cntRestaEnviar > maxCntAEnviarPorTrans? maxCntAEnviarPorTrans: cntRestaEnviar;

				// Configuro el envio
				DMAPS_ConfigSend((u32)nextBuffer, (u32)UART_TX_RX_FIFO_ADDR, 1, 4, cntEnviarAhora*sizeof(u8));

				// Envío
				DMAPS_Send();

				// Registro lo enviado
				nextBuffer += cntEnviarAhora;
				cntEnviados += cntEnviarAhora;
				cntRestaEnviar -= cntEnviarAhora;
			}
		}
	}

	int timeout = 10000;
	while(timeout --);

	xil_printf("&");
	LOG(0,"\nSe ejecutó correctamente el ejemplo");

	TAR_Start_master_test();
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
