/*
 * xuartps_0_xdmaps.c
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */
/***************************** Include Files *******************************/

#include "xuartps_0_xdmaps.h"
#include "xdmaps.h"

#include "xil_exception.h"
#include "xil_printf.h"

#include "interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void DMAPS_DoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);

/************************** Variable Definitions ***************************/

static XDmaPs 	DmaPs;

XDmaPs_Cmd DmaCmd;
volatile int Checked;
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
		0xA0
};

/****************************************************************************/

void DMAPS_Init(){
	XDmaPs *DmaPsPtr = &DmaPs;
	XDmaPs_Config *DmaCfg;

	DmaCfg = XDmaPs_LookupConfig(DMA_DEVICE_ID);

	XDmaPs_CfgInitialize(DmaPsPtr, DmaCfg, DmaCfg->BaseAddress);

	Xil_ExceptionInit();

	XDmaPs_SetDoneHandler(DmaPsPtr, DMA_CHANNEL, DMAPS_DoneHandler, (void *)&Checked);

	AddIntrHandler(&dmaFaultIntrConfig);
	AddIntrHandler(&dmaCh0IntrConfig);
}

void DMAPS_ConfigSend(u32 src, u32 dst, int burstSize, int burstLen, unsigned int transferLen){
	XDmaPs *DmaPsPtr = &DmaPs;

	// Confiuro el control de DMA para enviar datos desde el buffer Src al TX_FIFO de UART
	memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));

	DmaCmd.ChanCtrl.SrcBurstSize = burstSize;
	DmaCmd.ChanCtrl.SrcBurstLen = burstLen;
	DmaCmd.ChanCtrl.SrcInc = 1;
	DmaCmd.ChanCtrl.DstBurstSize = burstSize;
	DmaCmd.ChanCtrl.DstBurstLen = burstLen;
	DmaCmd.ChanCtrl.DstInc = 0;
	DmaCmd.BD.SrcAddr = src;
	DmaCmd.BD.DstAddr = dst;
	DmaCmd.BD.Length = transferLen;
}

void DMAPS_Send(){
	XDmaPs_Start(&DmaPs, DMA_CHANNEL, &DmaCmd, 0);
}

void DMAPS_DoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef){
	volatile int *Checked = (volatile int *)CallbackRef;

	*Checked = 1;
}

int DMAPS_Done(){
	int c = Checked;
	Checked = 0;
	return c;
}
