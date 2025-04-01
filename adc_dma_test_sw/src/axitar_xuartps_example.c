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
	u32 maxCntBYTES_EnviarPorTrans = sizeof(u32) * 2; // Mando datos de a 40 Bytes, son 10 valores dobles de 16b
	u32 cntBYTES_RestaEnviar = 0;
	u32 cntBYTES_EnviarAhora = 0;
	u32 cntBYTES_Enviados = 0;
	u32 *nextBuffer = (u32*)AXI_DMA_RX_BUFFER_BASE;


	// Enviar todos los datos de a 64 bytes (máximo de uart tx)
	cntBYTES_RestaEnviar = AXI_DMA_NUMBER_OF_TRANSFERS*sizeof(u32); // Cuantos BYTES me quedan por enviar
	cntBYTES_EnviarAhora = 0; // Desde donde respecto de la base
	cntBYTES_Enviados = 0;
	nextBuffer = (u32*)AXI_DMA_RX_BUFFER_BASE;

	LOG(1,"Enviando %d bytes de a %d bytes (%d pares) desde [0x%08x]",
			cntBYTES_RestaEnviar, maxCntBYTES_EnviarPorTrans, maxCntBYTES_EnviarPorTrans/sizeof(u32) , nextBuffer);

	xil_printf("&");
	while (cntBYTES_RestaEnviar) { //Continúo siempre que haya datos para enviar
		if(uart0DoneTx)	{
			if(DMAPS_Done()){
				uart0DoneTx = 0;
				usleep(800);

				// Calculo la cantidad a enviar en este lote según maxCntBYTES_EnviarPorTrans
				cntBYTES_EnviarAhora =  cntBYTES_RestaEnviar > maxCntBYTES_EnviarPorTrans? maxCntBYTES_EnviarPorTrans: cntBYTES_RestaEnviar;

				// Configuro el envio
				DMAPS_ConfigSend((u32)nextBuffer, (u32)UART_TX_RX_FIFO_ADDR, 1, 4, cntBYTES_EnviarAhora*sizeof(u8));

				// Envío
				DMAPS_Send();

				// Registro lo enviado
				nextBuffer += cntBYTES_EnviarAhora/sizeof(u32); // CANTIDAD EN DATOS NO EN BYTES!!!!
				cntBYTES_Enviados += cntBYTES_EnviarAhora;
				cntBYTES_RestaEnviar -= cntBYTES_EnviarAhora;
			}
		}
	}

	while(1){
		if(uart0DoneTx)	{
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

	LOG(0, "------------------------------------------------------------------------------------------------------------------");
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
