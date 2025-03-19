/*
 * dma_uart_example.c
 *
 *  Created on: Mar 18, 2025
 *      Author: sebas
 */

#include "xdmaps.h"
#include "xuartps.h"
#include "xscugic.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xil_types.h"
#include "xil_printf.h"
#include "xil_exception.h"
#include "interrupt_config.h"
#include "assert.h"
#include "log.h"

/*----------------------------------------------------------------*/

#define DMA_DEVICE_ID 		XPAR_XDMAPS_1_DEVICE_ID
#define DMA_DONE_INTR_0		XPAR_XDMAPS_0_DONE_INTR_0
#define DMA_FAULT_INTR		XPAR_XDMAPS_0_FAULT_INTR

//#define DMA_BUFFER_SIZE		1024	/* Length of the Dma Transfers */
#define DMA_BUFFER_SIZE		64
#define TIMEOUT_LIMIT 		0x2000	/* Loop count for timeout */

/*----------------------------------------------------------------*/

#define UART_DEVICE_ID      	XPAR_XUARTPS_0_DEVICE_ID
#define UART_INT_IRQ_ID     	XPAR_XUARTPS_0_INTR
#define UART_TX_RX_FIFO_ADDR	XPAR_XUARTPS_0_BASEADDR + XUARTPS_FIFO_OFFSET
#define UART_TX_RX_FIFO_SIZE	16
// El FIFO según el manual funciona como tx y rx fifo

/*----------------------------------------------------------------*/

//static u32 Src[DMA_BUFFER_SIZE] __attribute__ ((aligned (32)));
static uint8_t Src[DMA_BUFFER_SIZE] __attribute__ ((aligned (32))) =
		"Hola, esta es una prueba de DMA a UART0";

XDmaPs 	DmaInst;
XUartPs UartInst;		/* Instance of the UART Device */

/*----------------------------------------------------------------*/

int UART_Init(XUartPs *UartInstPtr, u16 DeviceId, u16 UartIntrId);
void UART_Handler(void *CallBackRef, u32 Event, unsigned int EventData);
void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr);

int DMA_Init(XDmaPs* DmaInstPtr, u16 DeviceId);
void DMA_SendBurst();
void DMA_DoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);

/*----------------------------------------------------------------*/

#define DMA_UART_EXAMPLE
#ifdef DMA_UART_EXAMPLE
int main()
{
	int i;

	// Configurar buffer de datos
//	for (i = 0; i < DMA_BUFFER_SIZE; i++) {
//		Src[i] = DMA_BUFFER_SIZE - i;
//	}

	// Configurar UART
	ASSERT_SUCCESS(
			UART_Init(&UartInst, UART_DEVICE_ID, UART_INT_IRQ_ID),
			"Error al configurar UART");

	// Configurar DMA
	ASSERT_SUCCESS(
			DMA_Init(&DmaInst, DMA_DEVICE_ID),
			"Error al configurar DMA");

	ASSERT_SUCCESS(
			SetupIntrSystem(),
			"Error al configurar interrupciones");

	// Iniciar transmisión
	DMA_SendBurst();

	while(1);

	LOG(3, "Prueba de transmisión de UART por DMA exitosa");

	return 0;
}
#endif // DMA_UART_EXAMPLE

/*----------------------------------------------------------------*/

int UART_Init(XUartPs *UartInstPtr, u16 DeviceId, u16 UartIntrId){
	LOG(2, "UART_Init");

	u32 IntrMask;
	XUartPs_Config *Config;
	Intr_Config intrConfig;

	Config = XUartPs_LookupConfig(DeviceId);
	ASSERT(Config != NULL, "No config found for %d", DeviceId);

	ASSERT_SUCCESS(
			XUartPs_CfgInitialize(UartInstPtr, Config, Config->BaseAddress),
			"Fallo configuración UART");

	/* Check hardware build */
	ASSERT_SUCCESS(
			XUartPs_SelfTest(UartInstPtr),
			"Fallo self test de UART");

	intrConfig.IntrId = UartIntrId;
	intrConfig.Handler = (Xil_ExceptionHandler)XUartPs_InterruptHandler_Wrapper;
	intrConfig.CallBackRef = (void *)UartInstPtr;
	AddIntrHandler(&intrConfig);

	XUartPs_SetHandler(UartInstPtr, (XUartPs_Handler)UART_Handler, UartInstPtr);

	IntrMask =
			XUARTPS_IXR_TOUT | XUARTPS_IXR_PARITY  | XUARTPS_IXR_FRAMING |
			XUARTPS_IXR_OVER | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL  |
			XUARTPS_IXR_RXOVR;

	if (UartInstPtr->Platform == XPLAT_ZYNQ_ULTRA_MP) {
		IntrMask |= XUARTPS_IXR_RBRK;
	}

	XUartPs_SetInterruptMask(UartInstPtr, IntrMask);

	XUartPs_SetOperMode(UartInstPtr, XUARTPS_OPER_MODE_NORMAL);

	XUartPs_SetRecvTimeout(UartInstPtr, 16);

	//XUartPs_SetFifoThreshold(XUartPs *InstancePtr, u8 TriggerLevel)

	return XST_SUCCESS;
}
void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr){
	u32 IsrStatus;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Read the interrupt ID register to determine which
	 * interrupt is active
	 */
	IsrStatus = XUartPs_ReadReg(InstancePtr->Config.BaseAddress,
				   XUARTPS_IMR_OFFSET);

	IsrStatus &= XUartPs_ReadReg(InstancePtr->Config.BaseAddress,
				   XUARTPS_ISR_OFFSET);

	if((IsrStatus & ((u32)XUARTPS_IXR_TXEMPTY)) != (u32)0) {
		//todo manejar interrupicón por TX_EMPTY
		//todo iniciar una nueva transacción DMA por 16 elementos
		DMA_SendBurst();
	}

	if((IsrStatus & ((u32)XUARTPS_IXR_TXFULL)) != (u32)0) {
		//todo manejar interrupicón por TX_FULL
	}

	// Por último llamo a la función original que debía estar conectada al evento
	// para no romper con la cadena que ejecuta UART_Handler
	XUartPs_InterruptHandler(InstancePtr);
}
void UART_Handler(void *CallBackRef, u32 Event, unsigned int EventData){
//	XUartPs *UartInstPtr = (XUartPs *) CallBackRef;
//
//	/* All of the data has been sent */
//	if (Event == XUARTPS_EVENT_SENT_DATA) {
//		TotalSentCount = EventData;
//	}
//
//	/* All of the data has been received */
//	if (Event == XUARTPS_EVENT_RECV_DATA ||
//		Event == XUARTPS_EVENT_RECV_TOUT ||
//		Event == XUARTPS_EVENT_RECV_ERROR ||
//		Event == XUARTPS_EVENT_PARE_FRAME_BRKE ||
//		Event == XUARTPS_EVENT_RECV_ORERR) {
//
//		XUartPs_Recv(UartInstPtr, RecvBuffer, TEST_BUFFER_SIZE);
//
////		TotalReceivedCount = EventData;
//
//		xil_printf("Datos: %s\n", RecvBuffer);
//		memset(RecvBuffer, 0, TEST_BUFFER_SIZE);
//	}
}

/*----------------------------------------------------------------*/

int DMA_Init(XDmaPs* DmaInstPtr, u16 DeviceId){
	LOG(2, "DMA_Init");
	XDmaPs_Config *Config;
	XDmaPs_Cmd DmaCmd;
	Intr_Config faultIntrConfig, ch0IntrConfig;


	memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));

	DmaCmd.ChanCtrl.SrcBurstSize = 1;
	DmaCmd.ChanCtrl.SrcBurstLen = 1;
	DmaCmd.ChanCtrl.SrcInc = 1;
	DmaCmd.ChanCtrl.DstBurstSize = 1;
	DmaCmd.ChanCtrl.DstBurstLen = 1;
	DmaCmd.ChanCtrl.DstInc = 0;
	DmaCmd.BD.SrcAddr = (u32) Src;
	DmaCmd.BD.DstAddr = (u32) UART_TX_RX_FIFO_ADDR;
	DmaCmd.BD.Length = UART_TX_RX_FIFO_SIZE;

	Config = XDmaPs_LookupConfig(DeviceId);
	ASSERT(Config != NULL, "No config found for %d", DeviceId);

	ASSERT_SUCCESS(
			XDmaPs_CfgInitialize(DmaInstPtr, Config, Config->BaseAddress),
			"Fallo al configurar DMA");

	faultIntrConfig.IntrId = DMA_FAULT_INTR;
	faultIntrConfig.Handler = (Xil_ExceptionHandler)XDmaPs_FaultISR;
	faultIntrConfig.CallBackRef = (void *)DmaInstPtr;
	AddIntrHandler(&faultIntrConfig);

	ch0IntrConfig.IntrId = DMA_DONE_INTR_0;
	ch0IntrConfig.Handler = (Xil_ExceptionHandler)XDmaPs_DoneISR_0;
	ch0IntrConfig.CallBackRef = (void *)DmaInstPtr;
	AddIntrHandler(&ch0IntrConfig);

	XDmaPs_SetDoneHandler(DmaInstPtr, 0, DMA_DoneHandler, (void *)DmaInstPtr);

	return XST_SUCCESS;
}
void DMA_SendBurst(){
	//todo XDmaPs_Start
}
void DMA_DoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef){
	/* done handler */
//	volatile int *Checked = (volatile int *)CallbackRef;
//	int Index;
//	int Status = 1;
//	int *Src;
//	int *Dst;
//
//	Src = (int *)DmaCmd->BD.SrcAddr;
//	Dst = (int *)DmaCmd->BD.DstAddr;
//
//	/* DMA successful */
//	/* compare the src and dst buffer */
//	for (Index = 0; Index < DMA_LENGTH; Index++) {
//		if ((Src[Index] != Dst[Index]) ||
//		    (Dst[Index] != DMA_LENGTH - Index)) {
//			Status = -XST_FAILURE;
//		}
//	}
//
//
//	Checked[Channel] = Status;
}

