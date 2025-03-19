/*
 * xdmaps_example_w_intr.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#include <stdio.h>
#include <stdlib.h>
#include "sleep.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xil_assert.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xdmaps.h"

/************************** Constant Definitions *****************************/
/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */

#define DMA_DEVICE_ID 			XPAR_XDMAPS_1_DEVICE_ID
#define INTC_DEVICE_ID			XPAR_SCUGIC_SINGLE_DEVICE_ID

#define DMA_DONE_INTR_0			XPAR_XDMAPS_0_DONE_INTR_0
#define DMA_DONE_INTR_1			XPAR_XDMAPS_0_DONE_INTR_1
#define DMA_DONE_INTR_2			XPAR_XDMAPS_0_DONE_INTR_2
#define DMA_DONE_INTR_3			XPAR_XDMAPS_0_DONE_INTR_3
#define DMA_DONE_INTR_4			XPAR_XDMAPS_0_DONE_INTR_4
#define DMA_DONE_INTR_5			XPAR_XDMAPS_0_DONE_INTR_5
#define DMA_DONE_INTR_6			XPAR_XDMAPS_0_DONE_INTR_6
#define DMA_DONE_INTR_7			XPAR_XDMAPS_0_DONE_INTR_7
#define DMA_FAULT_INTR			XPAR_XDMAPS_0_FAULT_INTR


#define TEST_ROUNDS		1		/* Number of loops that the Dma transfers run.*/
#define DMA_LENGTH		1024	/* Length of the Dma Transfers */
#define TIMEOUT_LIMIT 	0x2000	/* Loop count for timeout */


int XDmaPs_Example_W_Intr(XScuGic *GicPtr, u16 DeviceId);
int SetupInterruptSystem(XScuGic *GicPtr, XDmaPs *DmaPtr);
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd,
		    void *CallbackRef);


static int Src[DMA_LENGTH] __attribute__ ((aligned (32)));
static int Dst[DMA_LENGTH] __attribute__ ((aligned (32)));

XDmaPs DmaInstance;
XScuGic GicInstance;

#ifdef XDMAPS_EXAMPLE_W_INTR
int main(void)
{
	int Status;

	Status = XDmaPs_Example_W_Intr(&GicInstance, DMA_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Error: XDMaPs_Example_W_Intr failed\r\n");
		return XST_FAILURE;
	} else {
		xil_printf("Successfully ran XDMaPs_Example_W_Intr\r\n");
		return XST_SUCCESS;
	}

}
#endif // XDMAPS_EXAMPLE_W_INTR

int XDmaPs_Example_W_Intr(XScuGic *GicPtr, u16 DeviceId)
{
	int Index;
	unsigned int Channel = 0;
	int Status;
	int TestStatus;
	int TestRound;
	int TimeOutCnt;
	volatile int Checked[XDMAPS_CHANNELS_PER_DEV];
	XDmaPs_Config *DmaCfg;
	XDmaPs *DmapsInstPtr = &DmaInstance;
	XDmaPs_Cmd DmaCmd;

	memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));

	DmaCmd.ChanCtrl.SrcBurstSize = 4;
	DmaCmd.ChanCtrl.SrcBurstLen = 4;
	DmaCmd.ChanCtrl.SrcInc = 1;
	DmaCmd.ChanCtrl.DstBurstSize = 4;
	DmaCmd.ChanCtrl.DstBurstLen = 4;
	DmaCmd.ChanCtrl.DstInc = 1;
	DmaCmd.BD.SrcAddr = (u32) Src;
	DmaCmd.BD.DstAddr = (u32) Dst;
	DmaCmd.BD.Length = DMA_LENGTH * sizeof(int);


	/*
	 * Initialize the DMA Driver
	 */
	DmaCfg = XDmaPs_LookupConfig(DeviceId);
	if (DmaCfg == NULL) {
		return XST_FAILURE;
	}

	Status = XDmaPs_CfgInitialize(DmapsInstPtr,
				      DmaCfg,
				      DmaCfg->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Setup the interrupt system.
	 */

	Status = SetupInterruptSystem(GicPtr, DmapsInstPtr);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	TestStatus = XST_SUCCESS;

	for (TestRound = 0; TestRound < TEST_ROUNDS; TestRound++) {
		for (Channel = 0;
		     Channel < XDMAPS_CHANNELS_PER_DEV;
		     Channel++) {


			/* Initialize source */
			for (Index = 0; Index < DMA_LENGTH; Index++) {
				Src[Index] = DMA_LENGTH - Index;
			}

			/* Clear destination */
			for (Index = 0; Index < DMA_LENGTH; Index++) {
				Dst[Index] = 0;
			}

			Checked[Channel] = 0;

			/* Set the Done interrupt handler */
			XDmaPs_SetDoneHandler(DmapsInstPtr,
					      Channel,
					      DmaDoneHandler,
					      (void *)Checked);

			Status = XDmaPs_Start(DmapsInstPtr, Channel, &DmaCmd, 0);
			if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}

			TimeOutCnt = 0;

			/* Now the DMA is done */
			while (!Checked[Channel]
			       && TimeOutCnt < TIMEOUT_LIMIT) {
				TimeOutCnt++;
			}

			if (TimeOutCnt >= TIMEOUT_LIMIT) {
				TestStatus = XST_FAILURE;
			}

			if (Checked[Channel] < 0) {
				/* DMA controller failed */
				TestStatus = XST_FAILURE;
			}
		}
	}

	return TestStatus;

}

int SetupInterruptSystem(XScuGic *GicPtr, XDmaPs *DmaPtr)
{
	int Status;
	XScuGic_Config *GicConfig;


	Xil_ExceptionInit();

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */

	GicConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == GicConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(GicPtr, GicConfig,
				       GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				     (Xil_ExceptionHandler)XScuGic_InterruptHandler,
				     GicPtr);
	/*
	 * Connect the device driver handlers that will be called when an interrupt
	 * for the device occurs, the device driver handler performs the specific
	 * interrupt processing for the device
	 */

	/*
	 * Connect the Fault ISR
	 */
	Status = XScuGic_Connect(GicPtr,
				 DMA_FAULT_INTR,
				 (Xil_InterruptHandler)XDmaPs_FaultISR,
				 (void *)DmaPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the Done ISR for all 8 channels of DMA 0
	 */
	Status = XScuGic_Connect(GicPtr,
				 DMA_DONE_INTR_0,
				 (Xil_InterruptHandler)XDmaPs_DoneISR_0,
				 (void *)DmaPtr);
	Status |= XScuGic_Connect(GicPtr,
				  DMA_DONE_INTR_1,
				  (Xil_InterruptHandler)XDmaPs_DoneISR_1,
				  (void *)DmaPtr);
	Status |= XScuGic_Connect(GicPtr,
				  DMA_DONE_INTR_2,
				  (Xil_InterruptHandler)XDmaPs_DoneISR_2,
				  (void *)DmaPtr);
	Status |= XScuGic_Connect(GicPtr,
				  DMA_DONE_INTR_3,
				  (Xil_InterruptHandler)XDmaPs_DoneISR_3,
				  (void *)DmaPtr);
	Status |= XScuGic_Connect(GicPtr,
				  DMA_DONE_INTR_4,
				  (Xil_InterruptHandler)XDmaPs_DoneISR_4,
				  (void *)DmaPtr);
	Status |= XScuGic_Connect(GicPtr,
				  DMA_DONE_INTR_5,
				  (Xil_InterruptHandler)XDmaPs_DoneISR_5,
				  (void *)DmaPtr);
	Status |= XScuGic_Connect(GicPtr,
				  DMA_DONE_INTR_6,
				  (Xil_InterruptHandler)XDmaPs_DoneISR_6,
				  (void *)DmaPtr);
	Status |= XScuGic_Connect(GicPtr,
				  DMA_DONE_INTR_7,
				  (Xil_InterruptHandler)XDmaPs_DoneISR_7,
				  (void *)DmaPtr);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupts for the device
	 */
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_0);
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_1);
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_2);
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_3);
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_4);
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_5);
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_6);
	XScuGic_Enable(GicPtr, DMA_DONE_INTR_7);
	XScuGic_Enable(GicPtr, DMA_FAULT_INTR);

	Xil_ExceptionEnable();

	return XST_SUCCESS;

}

void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef)
{
	/* done handler */
	volatile int *Checked = (volatile int *)CallbackRef;
	int Index;
	int Status = 1;
	int *Src;
	int *Dst;

	Src = (int *)DmaCmd->BD.SrcAddr;
	Dst = (int *)DmaCmd->BD.DstAddr;

	/* DMA successful */
	/* compare the src and dst buffer */
	for (Index = 0; Index < DMA_LENGTH; Index++) {
		if ((Src[Index] != Dst[Index]) ||
		    (Dst[Index] != DMA_LENGTH - Index)) {
			Status = -XST_FAILURE;
		}
	}


	Checked[Channel] = Status;
}


