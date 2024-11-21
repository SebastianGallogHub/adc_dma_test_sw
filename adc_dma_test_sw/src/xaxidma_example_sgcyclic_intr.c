/******************************************************************************
* Copyright (C) 2018 - 2022 Xilinx, Inc.  All rights reserved.
* Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file xaxidma_example_sgcyclic_intr.c
 *
 * This file demonstrates how to use the xaxidma driver on the Xilinx AXI
 * DMA core (AXIDMA) to transfer packets in interrupt mode when the AXIDMA
 * core is configured in Scatter Gather Mode
 *
 * This example demonstrates how to use cyclic DMA mode feature.
 * This program will recycle the NUMBER_OF_BDS_TO_TRANSFER
 * buffer descriptors to specified number of cyclic transfers defined in
 * "NUMBER_OF_CYCLIC_TRANSFERS".
 *
 * This code assumes a loopback hardware widget is connected to the AXI DMA
 * core for data packet loopback.
 *
 * To see the debug print, you need a Uart16550 or uartlite in your system,
 * and please set "-DDEBUG" in your compiler options. You need to rebuild your
 * software executable.
 *
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 9.4   adk  25/07/17 Initial version.
 * 9.6   rsp  02/14/18 Support data buffers above 4GB.Use UINTPTR for storing
 *                     and typecasting buffer address(CR-992638).
 * 9.8   rsp  07/24/18 Set TX DMACR[Cyclic BD enable] before starting DMA
 *                     operation i.e. in TxSetup.
 * 9.9   rsp  01/21/19 Fix use of #elif check in deriving DDR_BASE_ADDR.
 *       rsp  02/05/19 For test completion wait for both TX and RX done counters.
 * 9.10  rsp  09/17/19 Fix cache maintenance ops for source and dest buffer.
 * 9.14  sk   03/08/22 Delete DDR memory limits comments as they are not
 * 		       relevant to this driver version.
 * 9.15  sa   08/12/22 Updated the example to use latest MIG cannoical define
 * 		       i.e XPAR_MIG_0_C0_DDR4_MEMORY_MAP_BASEADDR.
 * 9.16  sa   09/29/22 Fix infinite loops in the example.
 *
 * </pre>
 *
 * ***************************************************************************
 */
/***************************** Include Files *********************************/
#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xdebug.h"
#include "xil_util.h"

#include "log.h"
#include "assert.h"

#ifndef DEBUG
extern void xil_printf(const char *format, ...);
#endif

#include "xscugic.h"

/******************** Constant Definitions **********************************/
/*
 * Device hardware build related constants.
 */
#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID

#define MEM_BASE_ADDR		(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000)

#define RX_INTR_ID		XPAR_FABRIC_AXIDMA_0_S2MM_INTROUT_VEC_ID
#define TX_INTR_ID		XPAR_FABRIC_AXIDMA_0_MM2S_INTROUT_VEC_ID

#define RX_BD_SPACE_BASE	(MEM_BASE_ADDR)
#define RX_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0000FFFF)
#define TX_BD_SPACE_BASE	(MEM_BASE_ADDR + 0x00010000)
#define TX_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0001FFFF)
#define TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00100000)
#define TX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x002FFFFF)
#define RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00300000)
#define RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x004FFFFF)

#define INTC_DEVICE_ID          XPAR_SCUGIC_SINGLE_DEVICE_ID

/* Timeout loop counter for reset
 */
#define RESET_TIMEOUT_COUNTER	10000

/*
 * Buffer and Buffer Descriptor related constant definition
 */
#define MAX_PKT_LEN			0x100
#define MARK_UNCACHEABLE    0x701

/*
 * Number of BDs in the transfer example
 * We show how to submit multiple BDs for one transmit.
 * The receive side get one completion interrupt per cyclic transfer.
 */
#define NUMBER_OF_BDS_PER_PKT		2
#define NUMBER_OF_PKTS_TO_TRANSFER 	10
#define NUMBER_OF_BDS_TO_RECEIVE	5
#define NUMBER_OF_BDS_TO_TRANSFER	(NUMBER_OF_PKTS_TO_TRANSFER * NUMBER_OF_BDS_PER_PKT)

#define NUMBER_OF_CYCLIC_TRANSFERS	2
#define TX_SG_CYCLE
#define RX_SG_CYCLE

#ifndef TX_SG_CYCLE
#undef NUMBER_OF_CYCLIC_TRANSFERS
#define NUMBER_OF_CYCLIC_TRANSFERS 1
#endif

/* The interrupt coalescing threshold and delay timer threshold
 * Valid range is 1 to 255
 *
 * We set the coalescing threshold to be the total number of packets.
 * The receive side will only get one completion interrupt per cyclic transfer.
 */
#define COALESCING_COUNT		1//NUMBER_OF_PKTS_TO_TRANSFER
#define DELAY_TIMER_COUNT		100
#define POLL_TIMEOUT_COUNTER            1000000U
#define NUMBER_OF_EVENTS		1
#define INITIAL_VALUE			0xC

#ifndef SDT
#ifdef XPAR_INTC_0_DEVICE_ID
#define INTC		XIntc
#define INTC_HANDLER	XIntc_InterruptHandler
#else
#define INTC		XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler
#endif
#endif

/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/
#ifdef XPAR_UARTNS550_0_BASEADDR
static void Uart550_Setup(void);
#endif

static int CheckData(int Length, u8 StartValue);
static int CheckData2(int cant);
static void TxCallBack(XAxiDma_BdRing *TxRingPtr);
static void TxIntrHandler(void *Callback);
static void RxCallBack(XAxiDma_BdRing *RxRingPtr);
static void RxIntrHandler(void *Callback);


#ifndef SDT
static int SetupIntrSystem(INTC *IntcInstancePtr,
			   XAxiDma *AxiDmaPtr, u16 TxIntrId, u16 RxIntrId);
static void DisableIntrSystem(INTC *IntcInstancePtr,
			      u16 TxIntrId, u16 RxIntrId);
#endif

static int RxSetup(XAxiDma *AxiDmaInstPtr);
static int TxSetup(XAxiDma *AxiDmaInstPtr);
static int SendPacket(XAxiDma *AxiDmaInstPtr);

/************************** Variable Definitions *****************************/
/*
 * Device instance definitions
 */
static XAxiDma AxiDma;

#ifndef SDT
static INTC Intc;	/* Instance of the Interrupt Controller */
#endif

/*
 * Flags interrupt handlers use to notify the application context the events.
 */
static u32 TxDone;
static u32 RxDone;
static u32 Error;

/*
 * Buffer for transmit packet. Must be 32-bit aligned to be used by DMA.
 */
static u32 *Packet = (u32 *) TX_BUFFER_BASE;

/*****************************************************************************/
/**
*
* Main function
*
* This function is the main entry of the interrupt test. It does the following:
*	- Set up the output terminal if UART16550 is in the hardware build
*	- Initialize the DMA engine
*	- Set up Tx and Rx channels
*	- Set up the interrupt system for the Tx and Rx interrupts
*	- Submit a transfer
*	- Wait for the transfer to finish
*	- Check transfer status
*	- Disable Tx and Rx interrupts
*	- Print test status and exit
*
* @param	None
*
* @return
*		- XST_SUCCESS if tests pass
*		- XST_FAILURE if fails.
*
* @note		None.
*
******************************************************************************/
int sgcyclic(void)
{
	int Status;
	XAxiDma_Config *Config;
#ifdef SDT
	XAxiDma_BdRing *RxRingPtr;
	XAxiDma_BdRing *TxRingPtr;
#endif

	/* Initial setup for Uart16550 */
#ifdef XPAR_UARTNS550_0_BASEADDR

	Uart550_Setup();

#endif

	xil_printf("\r\n--- Entering main() --- \r\n");

#ifdef TX_SG_CYCLE
	LOG(2,"* Tx cíclico");
#else
	LOG(2,"* Tx normal");
#endif

#ifdef RX_SG_CYCLE
	LOG(2,"* Rx cíclico");
#else
	LOG(2,"* Rx normal");
#endif

	LOG(2,"");

#ifdef __aarch64__
	Xil_SetTlbAttributes(TX_BD_SPACE_BASE, MARK_UNCACHEABLE);
	Xil_SetTlbAttributes(RX_BD_SPACE_BASE, MARK_UNCACHEABLE);
#endif

#ifndef SDT
	Config = XAxiDma_LookupConfig(DMA_DEV_ID);
	if (!Config) {
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);

		return XST_FAILURE;
	}
#else
	Config = XAxiDma_LookupConfig(XPAR_XAXIDMA_0_BASEADDR);
	if (!Config) {
		xil_printf("No config found for %d\r\n", XPAR_XAXIDMA_0_BASEADDR);

		return XST_FAILURE;
	}
#endif

	/* Initialize DMA engine */
	XAxiDma_CfgInitialize(&AxiDma, Config);

	if (!XAxiDma_HasSg(&AxiDma)) {
		xil_printf("Device configured as Simple mode \r\n");
		return XST_FAILURE;
	}

	/* Set up TX/RX channels to be ready to transmit and receive packets */
	Status = TxSetup(&AxiDma);

	if (Status != XST_SUCCESS) {

		xil_printf("Failed TX setup\r\n");
		return XST_FAILURE;
	}

	Status = RxSetup(&AxiDma);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed RX setup\r\n");
		return XST_FAILURE;
	}

	/* Set up Interrupt system  */
#ifndef SDT
	Status = SetupIntrSystem(&Intc, &AxiDma, TX_INTR_ID, RX_INTR_ID);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed intr setup\r\n");
		return XST_FAILURE;
	}
#else
	TxRingPtr = XAxiDma_GetTxRing(&AxiDma);
	Status = XSetupInterruptSystem(TxRingPtr, &TxIntrHandler,
				       Config->IntrId[0], Config->IntrParent,
				       XINTERRUPT_DEFAULT_PRIORITY);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
#endif
#ifdef SDT
	RxRingPtr = XAxiDma_GetRxRing(&AxiDma);
	Status = XSetupInterruptSystem(RxRingPtr, &RxIntrHandler,
				       Config->IntrId[1], Config->IntrParent,
				       XINTERRUPT_DEFAULT_PRIORITY);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
#endif
	/* Initialize flags before start transfer test  */
	TxDone = 0;
	RxDone = 0;
	Error = 0;

	/* Send a packet */
	Status = SendPacket(&AxiDma);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed send packet\r\n");
		return XST_FAILURE;
	}

	/*
	 * Check for any error events to occur.
	 * Upon error, check the error path (Tx/Rx)
	 */
	Status = Xil_WaitForEventSet(POLL_TIMEOUT_COUNTER, NUMBER_OF_EVENTS, &Error);
	if (Status == XST_SUCCESS) {
		if (!TxDone) {
			xil_printf("Transmit error %d\r\n", Status);
			goto Done;
		} else if (!RxDone) {
			xil_printf("Receive error %d\r\n", Status);
			goto Done;
		}
	}

	/*
	 * Wait for TX done to complete transfer for all the BDs or timeout and
	 * Wait for RX done to be received for all the BDs or timeout
	 */

#ifdef RX_SG_CYCLE
	while (1) {
		if (((RxDone >= NUMBER_OF_BDS_TO_TRANSFER * NUMBER_OF_CYCLIC_TRANSFERS) ||
			 (TxDone >= NUMBER_OF_BDS_TO_TRANSFER * NUMBER_OF_CYCLIC_TRANSFERS))
			&& !Error) {
			break;
		}
	}
#else
	while (1) {
		if (((RxDone >= NUMBER_OF_BDS_TO_RECEIVE) ||
			 (TxDone >= NUMBER_OF_BDS_TO_TRANSFER * NUMBER_OF_CYCLIC_TRANSFERS))
			&& !Error) {
			break;
		}
	}
#endif

	XAxiDma_Reset(&AxiDma);

	/*
	 * Test finished, check data
	 */
	Status = CheckData2(NUMBER_OF_BDS_TO_TRANSFER);
	if (Status != XST_SUCCESS) {
		xil_printf("Data check failed\r\n");
		goto Done;
	}

//	LOG(1, "Enviados: %d",TxDone);
//	LOG(1, "Recibidos: %d",RxDone);
	xil_printf("Successfully ran AXI DMA Cyclic SG interrupt Example\r\n");

	/* Disable TX and RX Ring interrupts and return success */
#ifndef SDT
	DisableIntrSystem(&Intc, TX_INTR_ID, RX_INTR_ID);
#else
	XDisconnectInterruptCntrl(Config->IntrId[0], Config->IntrParent);
	XDisconnectInterruptCntrl(Config->IntrId[1], Config->IntrParent);
#endif

Done:

	LOG(1, "Enviados: %d",TxDone);
	LOG(1, "Recibidos: %d",RxDone);
	xil_printf("--- Exiting main() --- \r\n");

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

#ifdef XPAR_UARTNS550_0_BASEADDR
/*****************************************************************************/
/*
*
* Uart16550 setup routine, need to set baudrate to 9600 and data bits to 8
*
* @param	None
*
* @return	None
*
* @note		None.
*
******************************************************************************/
static void Uart550_Setup(void)
{

	XUartNs550_SetBaud(XPAR_UARTNS550_0_BASEADDR,
			   XPAR_XUARTNS550_CLOCK_HZ, 9600);

	XUartNs550_SetLineControlReg(XPAR_UARTNS550_0_BASEADDR,
				     XUN_LCR_8_DATA_BITS);
}
#endif

/*****************************************************************************/
/*
*
* This function checks data buffer after the DMA transfer is finished.
*
* We use the static tx/rx buffers.
*
* @param	Length is the length to check
* @param	StartValue is the starting value of the first byte
*
* @return	- XST_SUCCESS if validation is successful
*		- XST_FAILURE if validation fails.
*
* @note		None.
*
******************************************************************************/
static int CheckData(int Length, u8 StartValue)
{//MAX_PKT_LEN * NUMBER_OF_BDS_TO_TRANSFER, 0x0C
	u8 *RxPacket;
	int Index = 0,j=0;
//	u8 Value;

	u8 *TxPacket = (u8 *) TX_BUFFER_BASE;
	RxPacket = (u8 *) RX_BUFFER_BASE;
//	Value = StartValue;

	/* Invalidate the DestBuffer before receiving the data, in case the
	 * Data Cache is enabled
	 */
	Xil_DCacheInvalidateRange((UINTPTR)RxPacket, Length);


	LOG(1, "Checkeando:");
	LOG(2,"\t%s @ DIR\t|\t%s @ DIR", "Tx","Rx");
	for (Index = 0; Index < Length; Index++) {
		if(Index % NUMBER_OF_BDS_TO_TRANSFER == 0)
			LOG(2,"%d\t0x%2X @ 0x%X\t|\t0x%2X @ 0x%X", Index, TxPacket[Index], TxPacket+8*Index ,RxPacket[Index], RxPacket+8*Index);
//		j++;
		if (RxPacket[Index] != TxPacket[Index]) {
			xil_printf("Data error %d: %x/%x\r\n",
				   Index, RxPacket[Index], TxPacket[Index]);

			return XST_FAILURE;
		}
//		Value = (Value + 1) & 0xFF;
	}

	Index--;
	LOG(2,"%d\t0x%2X @ 0x%X\t|\t0x%2X @ 0x%X", Index, TxPacket[Index], TxPacket+8*Index ,RxPacket[Index], RxPacket+8*Index);

	return XST_SUCCESS;
}

static int CheckData2(int cant)
{
	u8 *RxPacket;
	int Index = 0,j=0;
//	u8 Value; UINTPTR RxBufferPtr

	u8 *TxPacket = (u8 *) TX_BUFFER_BASE;
	RxPacket = (u8 *) RX_BUFFER_BASE;
//	Value = StartValue;

	/* Invalidate the DestBuffer before receiving the data, in case the
	 * Data Cache is enabled
	 */
	Xil_DCacheInvalidateRange((UINTPTR)RxPacket, MAX_PKT_LEN * NUMBER_OF_BDS_TO_TRANSFER);


	LOG(1, "Checkeando:");
	LOG(4, "Tx @ DIR\t|\tRx @ DIR");
	for (Index = 0; Index < cant * MAX_PKT_LEN; Index++) {
		if(Index<=10)
			LOG(2,"%4d\t0x%04X @ 0x%X\t|\t0x%04X @ 0x%X", Index, TxPacket[Index], TxPacket+sizeof(u8)*Index ,RxPacket[Index], RxPacket+sizeof(u8)*Index);
	}
	Index--;
	LOG(2,"...");
	LOG(2,"%4d\t0x%04X @ 0x%X\t|\t0x%04X @ 0x%X", Index, TxPacket[Index], TxPacket+sizeof(u8)*Index ,RxPacket[Index], RxPacket+sizeof(u8)*Index);
	j = Index;
	LOG(2,"---");
	for (Index = 0; Index < 3; ++Index) {
		j++;
		LOG(2,"%4d\t0x%04X @ 0x%X\t|\t0x%04X @ 0x%X", j, TxPacket[j], TxPacket+sizeof(u8)*j,RxPacket[j], RxPacket+sizeof(u8)*j);
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/*
*
* This is the DMA TX callback function to be called by TX interrupt handler.
* This function handles BDs finished by hardware.
*
* @param	TxRingPtr is a pointer to TX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void TxCallBack(XAxiDma_BdRing *TxRingPtr)
{
	int BdCount;
	XAxiDma_Bd *BdPtr;

	/* Get all processed BDs from hardware */
	BdCount = XAxiDma_BdRingFromHw(TxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	TxDone += BdCount;

	/* When in cyclic mode, DMA processes BDs irrespective of ownership bit.
	 * This results in continuous interrupts. Disable TX interrupts once the
	 * required count of BDs are processed for this example, so that main
	 * non-interrupt thread can execute.
	 */
	if ((TxDone >= NUMBER_OF_BDS_TO_TRANSFER * NUMBER_OF_CYCLIC_TRANSFERS) && !Error) {
		XAxiDma_BdRingIntDisable(TxRingPtr, XAXIDMA_IRQ_ALL_MASK);
	}
}

/*****************************************************************************/
/*
*
* This is the DMA TX Interrupt handler function.
*
* It gets the interrupt status from the hardware, acknowledges it, and if any
* error happens, it resets the hardware. Otherwise, if a completion interrupt
* presents, then it calls the callback function.
*
* @param	Callback is a pointer to TX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void TxIntrHandler(void *Callback)
{
	XAxiDma_BdRing *TxRingPtr = (XAxiDma_BdRing *) Callback;
	u32 IrqStatus;
	int TimeOut;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_BdRingGetIrq(TxRingPtr);

	/* Acknowledge pending interrupts */
	XAxiDma_BdRingAckIrq(TxRingPtr, IrqStatus);

	/* If no interrupt is asserted, we do not do anything
	 */
	if (!(IrqStatus & XAXIDMA_IRQ_ALL_MASK)) {

		return;
	}

	/*
	 * If error interrupt is asserted, raise error flag, reset the
	 * hardware to recover from the error, and return with no further
	 * processing.
	 */
	if ((IrqStatus & XAXIDMA_IRQ_ERROR_MASK)) {

		XAxiDma_BdRingDumpRegs(TxRingPtr);

		Error = 1;

		/*
		 * Reset should never fail for transmit channel
		 */
		XAxiDma_Reset(&AxiDma);

		TimeOut = RESET_TIMEOUT_COUNTER;

		while (TimeOut) {
			if (XAxiDma_ResetIsDone(&AxiDma)) {
				break;
			}

			TimeOut -= 1;
		}

		return;
	}

	/*
	 * If Transmit done interrupt is asserted, call TX call back function
	 * to handle the processed BDs and raise the according flag
	 */
	if ((IrqStatus & (XAXIDMA_IRQ_DELAY_MASK | XAXIDMA_IRQ_IOC_MASK))) {
		TxCallBack(TxRingPtr);
	}
}

/*****************************************************************************/
/*
*
* This is the DMA RX callback function called by the RX interrupt handler.
* This function handles finished BDs by hardware, attaches new buffers to those
* BDs, and give them back to hardware to receive more incoming packets
*
* @param	RxRingPtr is a pointer to RX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void RxCallBack(XAxiDma_BdRing *RxRingPtr)
{
	//XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);
//	if ((RxDone >= NUMBER_OF_BDS_TO_TRANSFER * NUMBER_OF_CYCLIC_TRANSFERS) || Error)
//	if ((RxDone >= 1))
//		return;

	int BdCount;
	XAxiDma_Bd *BdPtr;

	/* Get finished BDs from hardware */
	BdCount = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	RxDone += BdCount;

	XAxiDma_BdRingFree(RxRingPtr, BdCount, BdPtr);
	BdCount = XAxiDma_BdRingGetFreeCnt(RxRingPtr);
	if(BdCount)
		ASSERT_void(XAxiDma_BdRingAlloc(RxRingPtr, BdCount, &BdPtr) == XST_SUCCESS, "Failed to alloc %d used Rx bds \r\n", BdCount);

	/* When in cyclic mode, DMA processes BDs irrespective of ownership bit.
	 * This results in continuous interrupts. Disable RX interrupts once the
	 * required count of BDs are processed for this example, so that main
	 * non-interrupt thread can execute.
	 */
	if ((RxDone >= NUMBER_OF_BDS_TO_TRANSFER * NUMBER_OF_CYCLIC_TRANSFERS) && !Error) {
		XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);
	}
}

/*****************************************************************************/
/*
*
* This is the DMA RX interrupt handler function
*
* It gets the interrupt status from the hardware, acknowledges it, and if any
* error happens, it resets the hardware. Otherwise, if a completion interrupt
* presents, then it calls the callback function.
*
* @param	Callback is a pointer to RX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void RxIntrHandler(void *Callback)
{
	XAxiDma_BdRing *RxRingPtr = (XAxiDma_BdRing *) Callback;
	u32 IrqStatus;
	int TimeOut;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_BdRingGetIrq(RxRingPtr);

	/* Acknowledge pending interrupts */
	XAxiDma_BdRingAckIrq(RxRingPtr, IrqStatus);

	/*
	 * If no interrupt is asserted, we do not do anything
	 */
	if (!(IrqStatus & XAXIDMA_IRQ_ALL_MASK)) {
		return;
	}

	/*
	 * If error interrupt is asserted, raise error flag, reset the
	 * hardware to recover from the error, and return with no further
	 * processing.
	 */
	if ((IrqStatus & XAXIDMA_IRQ_ERROR_MASK)) {

		XAxiDma_BdRingDumpRegs(RxRingPtr);

		Error = 1;

		/* Reset could fail and hang
		 * NEED a way to handle this or do not call it??
		 */
		XAxiDma_Reset(&AxiDma);

		TimeOut = RESET_TIMEOUT_COUNTER;

		while (TimeOut) {
			if (XAxiDma_ResetIsDone(&AxiDma)) {
				break;
			}

			TimeOut -= 1;
		}

		return;
	}

	/*
	 * If completion interrupt is asserted, call RX call back function
	 * to handle the processed BDs and then raise the according flag.
	 */
	if ((IrqStatus & (XAXIDMA_IRQ_DELAY_MASK | XAXIDMA_IRQ_IOC_MASK))) {
		RxCallBack(RxRingPtr);
	}
}
#ifndef SDT
/*****************************************************************************/
/*
*
* This function setups the interrupt system so interrupts can occur for the
* DMA, it assumes INTC component exists in the hardware system.
*
* @param	IntcInstancePtr is a pointer to the instance of the INTC.
* @param	AxiDmaPtr is a pointer to the instance of the DMA engine
* @param	TxIntrId is the TX channel Interrupt ID.
* @param	RxIntrId is the RX channel Interrupt ID.
*
* @return
*		- XST_SUCCESS if successful,
*		- XST_FAILURE.if not successful
*
* @note		None.
*
******************************************************************************/

int SetupIntrSystem(INTC *IntcInstancePtr,
			   XAxiDma *AxiDmaPtr, u16 TxIntrId, u16 RxIntrId)
{
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(AxiDmaPtr);
	XAxiDma_BdRing *RxRingPtr = XAxiDma_GetRxRing(AxiDmaPtr);
	int Status;

#ifdef XPAR_INTC_0_DEVICE_ID

	/* Initialize the interrupt controller and connect the ISRs */
	Status = XIntc_Initialize(IntcInstancePtr, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed init intc\r\n");
		return XST_FAILURE;
	}

	Status = XIntc_Connect(IntcInstancePtr, TxIntrId,
			       (XInterruptHandler) TxIntrHandler, TxRingPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed tx connect intc\r\n");
		return XST_FAILURE;
	}

	Status = XIntc_Connect(IntcInstancePtr, RxIntrId,
			       (XInterruptHandler) RxIntrHandler, RxRingPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed rx connect intc\r\n");
		return XST_FAILURE;
	}

	/* Start the interrupt controller */
	Status = XIntc_Start(IntcInstancePtr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed to start intc\r\n");
		return XST_FAILURE;
	}

	XIntc_Enable(IntcInstancePtr, TxIntrId);
	XIntc_Enable(IntcInstancePtr, RxIntrId);

#else

	XScuGic_Config *IntcConfig;


	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
				       IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	XScuGic_SetPriorityTriggerType(IntcInstancePtr, TxIntrId, 0xA0, 0x3);

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, RxIntrId, 0xA0, 0x3);
	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, TxIntrId,
				 (Xil_InterruptHandler)TxIntrHandler,
				 TxRingPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	Status = XScuGic_Connect(IntcInstancePtr, RxIntrId,
				 (Xil_InterruptHandler)RxIntrHandler,
				 RxRingPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	XScuGic_Enable(IntcInstancePtr, TxIntrId);
	XScuGic_Enable(IntcInstancePtr, RxIntrId);
#endif

	/* Enable interrupts from the hardware */

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				     (Xil_ExceptionHandler)INTC_HANDLER,
				     (void *)IntcInstancePtr);

	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function disables the interrupts for DMA engine.
*
* @param	IntcInstancePtr is the pointer to the INTC component instance
* @param	TxIntrId is interrupt ID associated w/ DMA TX channel
* @param	RxIntrId is interrupt ID associated w/ DMA RX channel
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void DisableIntrSystem(INTC *IntcInstancePtr,
			      u16 TxIntrId, u16 RxIntrId)
{
#ifdef XPAR_INTC_0_DEVICE_ID
	/* Disconnect the interrupts for the DMA TX and RX channels */
	XIntc_Disconnect(IntcInstancePtr, TxIntrId);
	XIntc_Disconnect(IntcInstancePtr, RxIntrId);
#else
	XScuGic_Disconnect(IntcInstancePtr, TxIntrId);
	XScuGic_Disconnect(IntcInstancePtr, RxIntrId);
#endif
}
#endif
/*****************************************************************************/
/*
*
* This function sets up RX channel of the DMA engine to be ready for packet
* reception
*
* @param	AxiDmaInstPtr is the pointer to the instance of the DMA engine.
*
* @return	- XST_SUCCESS if the setup is successful.
*		- XST_FAILURE if fails.
*
* @note		None.
*
******************************************************************************/
static int RxSetup(XAxiDma *AxiDmaInstPtr)
{
	XAxiDma_BdRing *RxRingPtr;
	int Status;
	XAxiDma_Bd BdTemplate;
	XAxiDma_Bd *BdPtr;
	XAxiDma_Bd *BdCurPtr;
	int BdCount;
	int FreeBdCount;
	UINTPTR RxBufferPtr;
	int Index;
	int bdSize;

	RxRingPtr = XAxiDma_GetRxRing(&AxiDma);

	/* Disable all RX interrupts before RxBD space setup */
	XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Setup Rx BD space */
	BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT,
					RX_BD_SPACE_HIGH - RX_BD_SPACE_BASE + 1);

	bdSize = (RX_BD_SPACE_HIGH-RX_BD_SPACE_BASE)/BdCount;

	LOG(2, "Se pueden configurar %d Bds para Rx", BdCount);

	Status = XAxiDma_BdRingCreate(RxRingPtr, RX_BD_SPACE_BASE,
				      RX_BD_SPACE_BASE,
				      XAXIDMA_BD_MINIMUM_ALIGNMENT, NUMBER_OF_BDS_TO_RECEIVE);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx bd create failed with %d\r\n", Status);
		return XST_FAILURE;
	}

	/*
	 * Setup a BD template for the Rx channel. Then copy it to every RX BD.
	 */
	XAxiDma_BdClear(&BdTemplate);
	Status = XAxiDma_BdRingClone(RxRingPtr, &BdTemplate);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx bd clone failed with %d\r\n", Status);
		return XST_FAILURE;
	}

	/* Attach buffers to RxBD ring so we are ready to receive packets */
	FreeBdCount = XAxiDma_BdRingGetFreeCnt(RxRingPtr);
	FreeBdCount=NUMBER_OF_BDS_TO_RECEIVE;

	Status = XAxiDma_BdRingAlloc(RxRingPtr, FreeBdCount, &BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx bd alloc failed with %d\r\n", Status);
		return XST_FAILURE;
	}

	BdCurPtr = BdPtr;
	RxBufferPtr = RX_BUFFER_BASE;

	LOG(2, "Se configuran %d Bds para Rx", FreeBdCount);
	LOG(2,"Bd configurados en memoria");
	LOG(3, "Desde: 0x%X\tHasta: 0x%X (%d de bds)\tLimite: 0x%X",
			(u8*)RX_BD_SPACE_BASE,
			(u8*)RX_BD_SPACE_BASE+FreeBdCount*bdSize,
			FreeBdCount,
			(u8*)RX_BD_SPACE_HIGH);
	LOG(2,"Destino de los datos en memoria");
	LOG(3, "Desde: 0x%X\tHasta: 0x%X (%d de slots de %db de largo = %d KB)",
			(u8*)RX_BUFFER_BASE,
			(u8*)(RX_BUFFER_BASE+MAX_PKT_LEN*FreeBdCount),
			FreeBdCount,
			MAX_PKT_LEN,
			B2KB(FreeBdCount*MAX_PKT_LEN));
	for (Index = 0; Index < FreeBdCount; Index++) {

		Status = XAxiDma_BdSetBufAddr(BdCurPtr, RxBufferPtr);
		if (Status != XST_SUCCESS) {
			xil_printf("Rx set buffer addr %x on BD %x failed %d\r\n",
				   (unsigned int)RxBufferPtr,
				   (UINTPTR)BdCurPtr, Status);

			return XST_FAILURE;
		}

		Status = XAxiDma_BdSetLength(BdCurPtr, MAX_PKT_LEN,
					     RxRingPtr->MaxTransferLen);
		if (Status != XST_SUCCESS) {
			xil_printf("Rx set length %d on BD %x failed %d\r\n",
				   MAX_PKT_LEN, (UINTPTR)BdCurPtr, Status);

			return XST_FAILURE;
		}

		/* Receive BDs do not need to set anything for the control
		 * The hardware will set the SOF/EOF bits per stream status
		 */
		XAxiDma_BdSetCtrl(BdCurPtr, 0);

		XAxiDma_BdSetId(BdCurPtr, RxBufferPtr);

//		*((u8*)RxBufferPtr) = 0;
//		if(Index%32==0) {LOG(3, "0x%04X @ 0x%X", *((u8*)RxBufferPtr), RxBufferPtr);}
		RxBufferPtr += MAX_PKT_LEN;
		BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RxRingPtr, BdCurPtr);
	}

	//Limpiando el buffer de llegada
	RxBufferPtr = RX_BUFFER_BASE;
	Index = 0;
	LOG(3,"Rx @ DIR");
	do
	{
		*((u8*)RxBufferPtr) = 0;
		if(Index<=10) {LOG(3, "0x%04X @ 0x%X", *((u8*)RxBufferPtr), RxBufferPtr);}
//		if(RxBufferPtr==0x14003E7)  {LOG(3, "0x%04X @ 0x%X**", *((u8*)RxBufferPtr), RxBufferPtr);}
		if((u8*)RxBufferPtr==(u8*)(RX_BUFFER_BASE+MAX_PKT_LEN*FreeBdCount))  {LOG(3,"..."); LOG(3, "0x%04X @ 0x%X**", *((u8*)RxBufferPtr), RxBufferPtr);}
		Index ++;
		RxBufferPtr += sizeof(u8);
	}while(RxBufferPtr <= RX_BUFFER_HIGH);
	RxBufferPtr -= sizeof(u8);
	LOG(3, "...");
	LOG(3, "0x%04X @ 0x%X", *((u8*)RxBufferPtr), RxBufferPtr);

	/*
	 * Set the coalescing threshold
	 *
	 * If you would like to have multiple interrupts to happen, change
	 * the COALESCING_COUNT to be a smaller value
	 */
	Status = XAxiDma_BdRingSetCoalesce(RxRingPtr, COALESCING_COUNT,
					   DELAY_TIMER_COUNT);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx set coalesce failed with %d\r\n", Status);
		return XST_FAILURE;
	}

	Status = XAxiDma_BdRingToHw(RxRingPtr, FreeBdCount, BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx ToHw failed with %d\r\n", Status);
		return XST_FAILURE;
	}

	/* Enable all RX interrupts */
	XAxiDma_BdRingIntEnable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Enable Cyclic DMA mode */
#ifdef RX_SG_CYCLE
	XAxiDma_BdRingEnableCyclicDMA(RxRingPtr);
	XAxiDma_SelectCyclicMode(AxiDmaInstPtr, XAXIDMA_DEVICE_TO_DMA, 1);
#endif //RX_SG_CYCLE

	/* Start RX DMA channel */
	Status = XAxiDma_BdRingStart(RxRingPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx start BD ring failed with %d\r\n", Status);
		return XST_FAILURE;
	}



	return XST_SUCCESS;
}

/*****************************************************************************/
/*
*
* This function sets up the TX channel of a DMA engine to be ready for packet
* transmission.
*
* @param	AxiDmaInstPtr is the pointer to the instance of the DMA engine.
*
* @return	- XST_SUCCESS if the setup is successful.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
static int TxSetup(XAxiDma *AxiDmaInstPtr)
{
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AxiDma);
	XAxiDma_Bd BdTemplate;
	int Status;

	/* Disable all TX interrupts before TxBD space setup */
	XAxiDma_BdRingIntDisable(TxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	Status = XAxiDma_BdRingCreate(TxRingPtr, TX_BD_SPACE_BASE,
				      TX_BD_SPACE_BASE,
				      XAXIDMA_BD_MINIMUM_ALIGNMENT, NUMBER_OF_BDS_TO_TRANSFER);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed create BD ring\r\n");
		return XST_FAILURE;
	}

	/*
	 * Like the RxBD space, we create a template and set all BDs to be the
	 * same as the template. The sender has to set up the BDs as needed.
	 */
	XAxiDma_BdClear(&BdTemplate);
	Status = XAxiDma_BdRingClone(TxRingPtr, &BdTemplate);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed clone BDs\r\n");
		return XST_FAILURE;
	}

	/*
	 * Set the coalescing threshold, so only one transmit interrupt
	 * occurs for this example
	 *
	 * If you would like to have multiple interrupts to happen, change
	 * the COALESCING_COUNT to be a smaller value
	 */
	Status = XAxiDma_BdRingSetCoalesce(TxRingPtr, COALESCING_COUNT,
					   DELAY_TIMER_COUNT);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed set coalescing"
			   " %d/%d\r\n", COALESCING_COUNT, DELAY_TIMER_COUNT);
		return XST_FAILURE;
	}

	/* Enable Cyclic DMA mode */
#ifdef TX_SG_CYCLE
	XAxiDma_BdRingEnableCyclicDMA(TxRingPtr);
	XAxiDma_SelectCyclicMode(AxiDmaInstPtr, XAXIDMA_DMA_TO_DEVICE, 1);
#endif //TX_SG_CYCLE

	/* Enable all TX interrupts */
	XAxiDma_BdRingIntEnable(TxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Start the TX channel */
	Status = XAxiDma_BdRingStart(TxRingPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed bd start\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/*
*
* This function non-blockingly transmits all packets through the DMA engine.
*
* @param	AxiDmaInstPtr points to the DMA engine instance
*
* @return
* 		- XST_SUCCESS if the DMA accepts all the packets successfully,
* 		- XST_FAILURE if error occurs
*
* @note		None.
*
******************************************************************************/
static int SendPacket(XAxiDma *AxiDmaInstPtr)
{
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(AxiDmaInstPtr);
	u8 *TxPacket;
	u8 Value;
	XAxiDma_Bd *BdPtr, *BdCurPtr;
	int Status;
	int Index, Pkts;
	UINTPTR BufferAddr;

	/*
	 * Each packet is limited to TxRingPtr->MaxTransferLen
	 *
	 * This will not be the case if hardware has store and forward built in
	 */
	if (MAX_PKT_LEN * NUMBER_OF_BDS_PER_PKT >
	    TxRingPtr->MaxTransferLen) {

		xil_printf("Invalid total per packet transfer length for the "
			   "packet %d/%d\r\n",
			   MAX_PKT_LEN * NUMBER_OF_BDS_PER_PKT,
			   TxRingPtr->MaxTransferLen);

		return XST_INVALID_PARAM;
	}

	TxPacket = (u8*)TX_BUFFER_BASE;
	Index = 0;
	LOG(3,"Tx @ DIR");
	do
	{
		*((u8*)TxPacket) = 0;
		if(Index<=10) {LOG(3, "0x%04X @ 0x%X", *((u8*)TxPacket), TxPacket);}
//		if(RxBufferPtr==0x14003E7)  {LOG(3, "0x%04X @ 0x%X**", *((u8*)RxBufferPtr), RxBufferPtr);}
		if((u8*)TxPacket==(u8*)(TX_BUFFER_BASE+MAX_PKT_LEN*NUMBER_OF_BDS_TO_TRANSFER))  {LOG(3,"..."); LOG(3, "0x%04X @ 0x%X**", *((u8*)TxPacket), TxPacket);}
		Index ++;
		TxPacket += sizeof(u8);
	}while(TxPacket <= (u8*)TX_BUFFER_HIGH);
	TxPacket -= sizeof(u8);
	LOG(3, "...");
	LOG(3, "0x%04X @ 0x%X", *((u8*)TxPacket), TxPacket);

	TxPacket = (u8 *) Packet;

	Value = 1;//INITIAL_VALUE;

	for (Index = 0; Index < MAX_PKT_LEN * NUMBER_OF_BDS_TO_TRANSFER; Index ++) {
		TxPacket[Index] = Value;

//		Value = (Value + 1) & 0xFF;
//		Value = (Index > NUMBER_OF_BDS_PER_PKT * MAX_PKT_LEN? 1 : 2);
//		Value = ((u8)(Index+1/MAX_PKT_LEN)) & 0xFF;
		if(Index >= (MAX_PKT_LEN * Value)-1) Value++;
	}

	/* Flush the buffers before the DMA transfer, in case the Data Cache
	 * is enabled
	 */
	Xil_DCacheFlushRange((UINTPTR)TxPacket, MAX_PKT_LEN *
			     NUMBER_OF_BDS_TO_TRANSFER);
	Xil_DCacheFlushRange((UINTPTR)RX_BUFFER_BASE, MAX_PKT_LEN *
			     NUMBER_OF_BDS_TO_TRANSFER);

	Status = XAxiDma_BdRingAlloc(TxRingPtr, NUMBER_OF_BDS_TO_TRANSFER,
				     &BdPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed bd alloc\r\n");
		return XST_FAILURE;
	}

	BufferAddr = (UINTPTR)Packet;
	BdCurPtr = BdPtr;

	/*
	 * Set up the BD using the information of the packet to transmit
	 * Each transfer has NUMBER_OF_BDS_PER_PKT BDs
	 */
	LOG(2,"...");
	LOG(2, "Se configuran %d Bds para Tx", NUMBER_OF_BDS_TO_TRANSFER);
	LOG(2,"Bd configurados en memoria");
	LOG(3, "Desde: 0x%X\tHasta: 0x%X (%d dbs)\tLímite: 0x%X",
			(u8*)TX_BD_SPACE_BASE,
			(u8*)(TX_BD_SPACE_BASE+NUMBER_OF_BDS_TO_TRANSFER*MAX_PKT_LEN),
			NUMBER_OF_BDS_TO_TRANSFER,
			(u8*)TX_BD_SPACE_HIGH);
	LOG(2,"Destino de los datos en memoria");
	LOG(3, "Desde: 0x%X\tHasta: 0x%X (%d paquetes de %d dbs = %d KB)",
			(u8*)TX_BUFFER_BASE,
			(u8*)(TX_BUFFER_BASE+NUMBER_OF_BDS_TO_TRANSFER*MAX_PKT_LEN),
			NUMBER_OF_PKTS_TO_TRANSFER,
			NUMBER_OF_BDS_PER_PKT,
			B2KB(NUMBER_OF_BDS_TO_TRANSFER*MAX_PKT_LEN));
	for (Index = 0; Index < NUMBER_OF_PKTS_TO_TRANSFER; Index++) {

		for (Pkts = 0; Pkts < NUMBER_OF_BDS_PER_PKT; Pkts++) {
			u32 CrBits = 0;

			Status = XAxiDma_BdSetBufAddr(BdCurPtr, BufferAddr);
			if (Status != XST_SUCCESS) {
				xil_printf("Tx set buffer addr %x on BD %x failed %d\r\n",
					   (unsigned int)BufferAddr,
					   (UINTPTR)BdCurPtr, Status);

				return XST_FAILURE;
			}

			Status = XAxiDma_BdSetLength(BdCurPtr, MAX_PKT_LEN,
						     TxRingPtr->MaxTransferLen);
			if (Status != XST_SUCCESS) {
				xil_printf("Tx set length %d on BD %x failed %d\r\n",
					   MAX_PKT_LEN, (UINTPTR)BdCurPtr, Status);

				return XST_FAILURE;
			}

			if (Pkts == 0) {
				/* The first BD has SOF set
				 */
				CrBits |= XAXIDMA_BD_CTRL_TXSOF_MASK;
#ifndef SDT
#if (XPAR_AXIDMA_0_SG_INCLUDE_STSCNTRL_STRM == 1)
				/* The first BD has total transfer length set
				 * in the last APP word, this is for the
				 * loopback widget
				 */
				Status = XAxiDma_BdSetAppWord(BdCurPtr,
							      XAXIDMA_LAST_APPWORD,
							      MAX_PKT_LEN * NUMBER_OF_BDS_PER_PKT);

				if (Status != XST_SUCCESS) {
					xil_printf("Set app word failed with %d\r\n",
						   Status);
				}
#endif
#else
				if (TxRingPtr->HasStsCntrlStrm) {

					/* The first BD has total transfer length set
					 * in the last APP word, this is for the
					 * loopback widget
					 */
					Status = XAxiDma_BdSetAppWord(BdCurPtr,
								      XAXIDMA_LAST_APPWORD,
								      MAX_PKT_LEN * NUMBER_OF_BDS_PER_PKT);

					if (Status != XST_SUCCESS) {
						xil_printf("Set app word failed with %d\r\n",
							   Status);
					}
				}
#endif
			}

			if (Pkts == (NUMBER_OF_BDS_PER_PKT - 1)) {
				/* The last BD should have EOF and IOC set
				 */
				CrBits |= XAXIDMA_BD_CTRL_TXEOF_MASK;
			}

			XAxiDma_BdSetCtrl(BdCurPtr, CrBits);
			XAxiDma_BdSetId(BdCurPtr, BufferAddr);

			BufferAddr += MAX_PKT_LEN;
			BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(TxRingPtr, BdCurPtr);
		}
	}

	/* Give the BD to hardware */
	Status = XAxiDma_BdRingToHw(TxRingPtr, NUMBER_OF_BDS_TO_TRANSFER,
				    BdPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed to hw, length %d\r\n",
			   (int)XAxiDma_BdGetLength(BdPtr,
						    TxRingPtr->MaxTransferLen));

		return XST_FAILURE;
	}

	return XST_SUCCESS;
}
