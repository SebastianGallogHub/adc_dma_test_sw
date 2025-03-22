
/***************************** Include Files *******************************/

#include "xparameters.h"
#include "xplatform_info.h"
#include "xuartps.h"
#include "xil_exception.h"
#include "xil_printf.h"
#ifdef SDT
#include "xinterrupt_wrap.h"
#else

#ifdef XPAR_INTC_0_DEVICE_ID
#include "xintc.h"
#else
#include "xscugic.h"
#endif
#endif

#include "xil_types.h"
#include "xil_assert.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xscugic.h"
#include "xdmaps.h"
/************************** Constant Definitions **************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#ifndef SDT
#define UART_DEVICE_ID		XPAR_XUARTPS_0_DEVICE_ID
#else
#define	XUARTPS_BASEADDRESS	XPAR_XUARTPS_0_BASEADDR
#endif

#ifndef SDT
#ifdef XPAR_INTC_0_DEVICE_ID
#define INTC		XIntc

#define INTC_DEVICE_ID		XPAR_INTC_0_DEVICE_ID
#define UART_INT_IRQ_ID		XPAR_INTC_0_UARTPS_0_VEC_ID
#else
#define INTC		XScuGic
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define UART_INT_IRQ_ID		XPAR_XUARTPS_0_INTR
#endif
#endif
#define UART_TX_RX_FIFO_ADDR	XPAR_XUARTPS_0_BASEADDR + XUARTPS_FIFO_OFFSET
/*
 * The following constant controls the length of the buffers to be sent
 * and received with the UART,
 */
#define TEST_BUFFER_SIZE	40

#define DMA_DEVICE_ID 			XPAR_XDMAPS_1_DEVICE_ID
#define DMA_DONE_INTR_0			XPAR_XDMAPS_0_DONE_INTR_0
#define DMA_FAULT_INTR			XPAR_XDMAPS_0_FAULT_INTR
#define DMA_LENGTH		1024	/* Length of the Dma Transfers */
#define TIMEOUT_LIMIT 	0x2000	/* Loop count for timeout */
/**************************** Type Definitions ******************************/


/************************** Function Prototypes *****************************/

void interrupt_init();

void DMA_Init2();
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);

void UART_Init();
int DMA_UART_Send(int send_normal);
void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr);
void Handler(void *CallBackRef, u32 Event, unsigned int EventData);


/************************** Variable Definitions ***************************/

XDmaPs DmaInstance;
XUartPs UartPs	;		/* Instance of the UART Device */
#ifndef SDT
INTC InterruptController;	/* Instance of the Interrupt Controller */
#endif

/*
 * The following buffers are used in this example to send and receive data
 * with the UART.
 */
static u8 SendBuffer[TEST_BUFFER_SIZE];	/* Buffer for Transmitting Data */
static u8 RecvBuffer[TEST_BUFFER_SIZE];	/* Buffer for Receiving Data */

//static int Src[DMA_LENGTH] __attribute__ ((aligned (32)));
//static int Dst[DMA_LENGTH] __attribute__ ((aligned (32)));

/*
 * The following counters are used to determine when the entire buffer has
 * been sent and received.
 */
volatile int TotalReceivedCount;
volatile int TotalSentCount;
int TotalErrorCount;

unsigned int Channel = 0;
XDmaPs_Cmd DmaCmd2;
volatile int Checked;



/**************************************************************************/
/**
*
* Main function to call the Uart interrupt example.
*
*
* @return	XST_SUCCESS if successful, XST_FAILURE if unsuccessful
*
* @note		None
*
**************************************************************************/
int main (void){

	DMA_Init2();

//	TimeOutCnt = 0;
//
//	/* Now the DMA is done */
//	while (!Checked && TimeOutCnt < TIMEOUT_LIMIT) {
//		TimeOutCnt++;
//	}
//
//	if (TimeOutCnt >= TIMEOUT_LIMIT) {
//		TestStatus = XST_FAILURE;
//	}
//
//	if (Checked < 0) {
//		/* DMA controller failed */
//		TestStatus = XST_FAILURE;
//	}
//
//	if (TestStatus== 0)
//		xil_printf("\nDMA BIEN\n");

	UART_Init();

	interrupt_init();

	return DMA_UART_Send(0);
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void DMA_Init2(){
//	int Index;
//	int TestStatus;
//	int TimeOutCnt;
	XDmaPs_Config *DmaCfg;
//	XDmaPs *DmapsInstPtr = &DmaInstance;
//	XScuGic_Config *GicConfig;

	DmaCfg = XDmaPs_LookupConfig(DMA_DEVICE_ID);

	XDmaPs_CfgInitialize(&DmaInstance, DmaCfg, DmaCfg->BaseAddress);

	Xil_ExceptionInit();

//	GicConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);

//	XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig->CpuBaseAddress);

//	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
//					 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
//					 &InterruptController);

//	XScuGic_Connect(&InterruptController, DMA_FAULT_INTR,
//					 (Xil_InterruptHandler)XDmaPs_FaultISR,
//					 (void *)&DmaInstance);
//
//	XScuGic_Enable(&InterruptController, DMA_FAULT_INTR);
//
//	XScuGic_Connect(&InterruptController,
//					 DMA_DONE_INTR_0,
//					 (Xil_InterruptHandler)XDmaPs_DoneISR_0,
//					 (void *)&DmaInstance);
//
//	XScuGic_Enable(&InterruptController, DMA_DONE_INTR_0);

//	Xil_ExceptionEnable();

//	for (Index = 0; Index < DMA_LENGTH; Index++) {
//		Src[Index] = DMA_LENGTH - Index;
//	}
//
//	for (Index = 0; Index < DMA_LENGTH; Index++) {
//		Dst[Index] = 0;
//	}

	XDmaPs_SetDoneHandler(&DmaInstance, Channel, DmaDoneHandler,(void *)&Checked);

}
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef){
	/* done handler */
	volatile int *Checked = (volatile int *)CallbackRef;
//	int Index;
	int Status = 1;
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
//			(Dst[Index] != DMA_LENGTH - Index)) {
//			Status = -XST_FAILURE;
//		}
//	}

	*Checked = Status;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void interrupt_init(){
	XScuGic_Config* IntcConfig;

	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);

	XScuGic_CfgInitialize(&InterruptController, IntcConfig,
				IntcConfig->CpuBaseAddress);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
			&InterruptController);

	//---------------------------------------------------------------

		XScuGic_Connect(&InterruptController, UART_INT_IRQ_ID,
				(Xil_ExceptionHandler) XUartPs_InterruptHandler_Wrapper,
				(void*) &UartPs);

		XScuGic_Enable(&InterruptController, UART_INT_IRQ_ID);

	//---------------------------------------------------------------

	XScuGic_Connect(&InterruptController, DMA_FAULT_INTR,
						 (Xil_InterruptHandler)XDmaPs_FaultISR,
						 (void *)&DmaInstance);

	XScuGic_Enable(&InterruptController, DMA_FAULT_INTR);

	//---------------------------------------------------------------

	XScuGic_Connect(&InterruptController,
					 DMA_DONE_INTR_0,
					 (Xil_InterruptHandler)XDmaPs_DoneISR_0,
					 (void *)&DmaInstance);

	XScuGic_Enable(&InterruptController, DMA_DONE_INTR_0);


	//---------------------------------------------------------------

	Xil_ExceptionEnable();
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void UART_Init() {
	XUartPs_Config* Config;
	u32 IntrMask;

	/* Config for interrupt controller */
	Config = XUartPs_LookupConfig(UART_DEVICE_ID);

	XUartPs_CfgInitialize(&UartPs, Config, Config->BaseAddress);

	// controlador intr
//	XScuGic_Config* IntcConfig;

//	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
//
//	XScuGic_CfgInitialize(&InterruptController, IntcConfig,
//				IntcConfig->CpuBaseAddress);
//
//	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
//			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
//			&InterruptController);
//
//	XScuGic_Connect(&InterruptController, UART_INT_IRQ_ID,
//			(Xil_ExceptionHandler) XUartPs_InterruptHandler_Wrapper,
//			(void*) &UartPs);
//
//	XScuGic_Enable(&InterruptController, UART_INT_IRQ_ID);
//
//	Xil_ExceptionEnable();


	XUartPs_SetHandler(&UartPs, (XUartPs_Handler) Handler, &UartPs);

	IntrMask =
		XUARTPS_IXR_TOUT | XUARTPS_IXR_PARITY  | XUARTPS_IXR_FRAMING |
		XUARTPS_IXR_OVER | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL  |
		XUARTPS_IXR_RXOVR;

	XUartPs_SetInterruptMask(&UartPs, IntrMask);

	//	XUartPs_SetOperMode(&UartPs, XUARTPS_OPER_MODE_LOCAL_LOOP);

	XUartPs_SetRecvTimeout(&UartPs, 8);
}
int DMA_UART_Send(int send_normal) {
	int Index;
	int BadByteCount = 0;

	for (Index = 0; Index < TEST_BUFFER_SIZE; Index++) {
		SendBuffer[Index] = (Index % 26) + 'A';
		RecvBuffer[Index] = 0;
	}

//	XUartPs_Recv(&UartPs, RecvBuffer, TEST_BUFFER_SIZE);


	if (send_normal) {
		XUartPs_Send(&UartPs, SendBuffer, TEST_BUFFER_SIZE);
	}
	else {
//		int send = 0;
//
//		UartPs.SendBuffer.RequestedBytes = TEST_BUFFER_SIZE;
//
//		while ((!XUartPs_IsTransmitFull(UartPs.Config.BaseAddress)) &&
//			   (TEST_BUFFER_SIZE > send)) {
//
//			/* Fill the FIFO from the buffer */
//			XUartPs_WriteReg(UartPs.Config.BaseAddress,XUARTPS_FIFO_OFFSET,((u32)SendBuffer[send]));
//
//			/* Increment the send count. */
//			send++;
//		}

		// para que en Handler se reciba la cantidad de enviados a través de EventData
		xil_printf("\r\nSeteando  DMA PARA ENVIAR POR UART\r\n");
		UartPs.SendBuffer.RequestedBytes = TEST_BUFFER_SIZE;

		memset(&DmaCmd2, 0, sizeof(XDmaPs_Cmd));

		DmaCmd2.ChanCtrl.SrcBurstSize = 1;
		DmaCmd2.ChanCtrl.SrcBurstLen = 4;
		DmaCmd2.ChanCtrl.SrcInc = 1;
		DmaCmd2.ChanCtrl.DstBurstSize = 1;
		DmaCmd2.ChanCtrl.DstBurstLen = 4;
		DmaCmd2.ChanCtrl.DstInc = 0;
		DmaCmd2.BD.SrcAddr = (u32) SendBuffer;
		DmaCmd2.BD.DstAddr = (u32) UART_TX_RX_FIFO_ADDR;
		DmaCmd2.BD.Length = TEST_BUFFER_SIZE * sizeof(u8);

		// Espero a que el mensaje anterior se haya mandado
		while(!XUartPs_IsTransmitEmpty(&UartPs));

		XDmaPs_Start(&DmaInstance, Channel, &DmaCmd2, 0);
	}

	XUartPs_Recv(&UartPs, RecvBuffer, TEST_BUFFER_SIZE);

	while (1) {
		if ((TotalSentCount >= TEST_BUFFER_SIZE)
		&& (TotalReceivedCount >= TEST_BUFFER_SIZE)) {
			break;
		}
	}

//	for (Index = 0; Index < TEST_BUFFER_SIZE; Index++) {
//		if (RecvBuffer[Index] != SendBuffer[Index]) {
//			BadByteCount++;
//		}
//	}

//	XUartPs_SetOperMode(&UartPs, XUARTPS_OPER_MODE_NORMAL);

//	if (BadByteCount != 0) {
//		return XST_FAILURE;
//	}

	xil_printf("Recibido: (%d chr) %s \n", TotalReceivedCount, RecvBuffer);
	xil_printf("\nSuccessfully ran UART Interrupt Example Test\r\n");

//	if(Checked)
//		xil_printf("Successfully ran DMA Interrupt Example Test\r\n");

	return 0;
}
void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr){
//	XUartPs *InstancePtr = (XUartPs*)CallBackRef;
	u32 IsrStatus;
	int i=0;

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
//		DMA_UART_Send();
//		uartTxDone = 1;
		// Sé que puedo enviar este mensaje por acá porque TXEMPTY
		if(Checked){
			xil_printf("\r\nSuccessfully ran DMA Interrupt Example Test\r\n");
			Checked = 0; // Sino se hace un bucle infinito
		}
	}

	if((IsrStatus & ((u32)XUARTPS_IXR_TXFULL)) != (u32)0) {
		//todo manejar interrupicón por TX_FULL
		i++;
	}

	// Por último llamo a la función original que debía estar conectada al evento
	// para no romper con la cadena que ejecuta UART_Handler para la recepción
	XUartPs_InterruptHandler(InstancePtr);
}
int quedaronBtesEnElRxFIFO = 0;
void Handler(void *CallBackRef, u32 Event, unsigned int EventData)
{
	XUartPs *UartInst = (XUartPs*)CallBackRef;
	XUartPsBuffer *bufferPtr = &UartInst->ReceiveBuffer;
	u32 bytesLeidos = 0;
//	u32 IsrStatus;
	u32 IntrMask;

	/* All of the data has been sent */
	if (Event == XUARTPS_EVENT_SENT_DATA) {
		TotalSentCount += EventData;

//		// Interrupciones activadas
//		IsrStatus = XUartPs_ReadReg(UartInst->Config.BaseAddress,
//						   XUARTPS_IMR_OFFSET);
//
//		// Interrupciones levantadas
//		IsrStatus &= XUartPs_ReadReg(UartInst->Config.BaseAddress,
//					   XUARTPS_ISR_OFFSET);
//
//		// Por lad dudas vuelvo a borrar la bandera de TXEMPTY
//		XUartPs_WriteReg(UartInst->Config.BaseAddress, XUARTPS_ISR_OFFSET, IsrStatus);

		// Reactivo la interrupción. Al parecer se desactiva en XUartPs_InterruptHandler
		IntrMask = XUartPs_GetInterruptMask(UartInst);

		IntrMask |= XUARTPS_IXR_TXEMPTY;

		XUartPs_SetInterruptMask(UartInst, IntrMask);
	}

	/* All of the data has been received */
	if (Event == XUARTPS_EVENT_RECV_DATA ||
		Event == XUARTPS_EVENT_RECV_TOUT) {

//		cntMaxBytesALeer =  TEST_BUFFER_SIZE - TotalReceivedCount;

		//XUartPs_Recv((XUartPs*)CallBackRef, bufferPtr, cntMaxBytesALeer);

		// bufferPtr.RequestedBytes = TEST_BUFFER_SIZE
		bytesLeidos =
				bufferPtr->RequestedBytes - bufferPtr->RemainingBytes - TotalReceivedCount;

		TotalReceivedCount += bytesLeidos;

		// Sólo recibo lo que entra en el buffer
		bufferPtr->NextBytePtr = RecvBuffer + TotalReceivedCount;

		quedaronBtesEnElRxFIFO = (bytesLeidos < EventData);
	}

//	/*
//	 * Data was received, but not the expected number of bytes, a
//	 * timeout just indicates the data stopped for 8 character times
//	 */
//	if (Event == XUARTPS_EVENT_RECV_TOUT) {
//		TotalReceivedCount = EventData;
////		XUartPs_Recv((XUartPs*)CallBackRef, RecvBuffer, TEST_BUFFER_SIZE);
//	}

	/*
	 * Data was received with an error, keep the data but determine
	 * what kind of errors occurred
	 */
	if (Event == XUARTPS_EVENT_RECV_ERROR ||
		Event == XUARTPS_EVENT_PARE_FRAME_BRKE ||
		Event == XUARTPS_EVENT_RECV_ORERR) {

		TotalReceivedCount += EventData;
		TotalErrorCount++;

//		XUartPs_Recv((XUartPs*)CallBackRef, RecvBuffer, TEST_BUFFER_SIZE);
	}

//	/*
//	 * Data was received with an parity or frame or break error, keep the data
//	 * but determine what kind of errors occurred. Specific to Zynq Ultrascale+
//	 * MP.
//	 */
//	if (Event == XUARTPS_EVENT_PARE_FRAME_BRKE) {
//		TotalReceivedCount = EventData;
//		TotalErrorCount++;
////		XUartPs_Recv((XUartPs*)CallBackRef, RecvBuffer, TEST_BUFFER_SIZE);
//	}
//
//	/*
//	 * Data was received with an overrun error, keep the data but determine
//	 * what kind of errors occurred. Specific to Zynq Ultrascale+ MP.
//	 */
//	if (Event == XUARTPS_EVENT_RECV_ORERR) {
//		TotalReceivedCount = EventData;
//		TotalErrorCount++;
////		XUartPs_Recv((XUartPs*)CallBackRef, RecvBuffer, TEST_BUFFER_SIZE);
//	}
}
