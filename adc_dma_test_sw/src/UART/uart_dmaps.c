/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : uart_dmaps.c
 * Descripción         : Archivo de implementación de herramientas para
 * 						 transferencias de datos de memoria a periférico
 * 						 mediante canal DMAPS a UART.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 24/03/2025
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
#include "../UART/uart_dmaps.h"

#include "xdmaps.h"
#include "xil_exception.h"
#include "xil_printf.h"

#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
void DMAPS_DoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);

/************************** Variable Definitions ***************************/
static XDmaPs 		DmaPs;
static XDmaPs_Cmd 	DmaCmd;

volatile int dmaPsDone = 1;
unsigned int Channel = 0;

Intr_Config dmaFaultIntrConfig = {
		DMA_FAULT_INTR,
		(Xil_ExceptionHandler)XDmaPs_FaultISR,
		(void *)&DmaPs,
		0xA0
};

Intr_Config dmaCh0IntrConfig = {
		DMA_DONE_INTR_0,
		(Xil_ExceptionHandler)XDmaPs_DoneISR_0,
		(void *)&DmaPs,
		0x08
};

/****************************************************************************/

void DMAPS_Init(){
	XDmaPs *DmaPsPtr = &DmaPs;
	XDmaPs_Config *DmaCfg;

	DmaCfg = XDmaPs_LookupConfig(DMA_DEVICE_ID);

	XDmaPs_CfgInitialize(DmaPsPtr, DmaCfg, DmaCfg->BaseAddress);

	XDmaPs_SetDoneHandler(DmaPsPtr, DMA_CHANNEL, DMAPS_DoneHandler, (void *)&dmaPsDone);

	IntrSystem_AddHandler(&dmaFaultIntrConfig);
	IntrSystem_AddHandler(&dmaCh0IntrConfig);

	// Configurar el formato del envío de datos
	memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));

	DmaCmd.ChanCtrl.SrcBurstSize = 1;
	DmaCmd.ChanCtrl.SrcBurstLen = 16;
	DmaCmd.ChanCtrl.SrcInc = 1;

	DmaCmd.ChanCtrl.DstBurstSize = 1;
	DmaCmd.ChanCtrl.DstBurstLen = 16;
	DmaCmd.ChanCtrl.DstInc = 0;
}

void DMAPS_ConfigSend(u32 src, u32 dst, unsigned int transferLen){
	// Configurar la fuente y destino de datos y la cantidad en bytes
	DmaCmd.BD.SrcAddr = src;
	DmaCmd.BD.DstAddr = dst;
	DmaCmd.BD.Length = transferLen;
}

void DMAPS_Send(){
	dmaPsDone = 0;
	XDmaPs_Start(&DmaPs, DMA_CHANNEL, &DmaCmd, 0);
}

void DMAPS_DoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef){
	volatile int *dmaPsDone = (volatile int *)CallbackRef;

	*dmaPsDone = 1;
}

int DMAPS_Done(){
	return dmaPsDone;
}
