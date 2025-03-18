/*
 * uart_config.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#include "uart_config.h"
#include "xparameters.h"
#include "xuartps.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"

#define INTC				XScuGic
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID

#define TEST_BUFFER_SIZE	100


int UartPsIntrExample(INTC *IntcInstPtr, XUartPs *UartInstPtr,u16 DeviceId, u16 UartIntrId);
static int SetupInterruptSystem(INTC *IntcInstancePtr, XUartPs *UartInstancePtr, u16 UartIntrId);
void Handler(void *CallBackRef, u32 Event, unsigned int EventData);

XUartPs UartPs	;		/* Instance of the UART Device */
INTC InterruptController;	/* Instance of the Interrupt Controller */

static u8 RecvBuffer[TEST_BUFFER_SIZE];	/* Buffer for Receiving Data */

volatile int TotalReceivedCount;
volatile int TotalSentCount;


//int main(void)
//{
//	int Status;
//
//	/* Run the UartPs Interrupt example, specify the the Device ID */
//	Status = UartPsIntrExample(&InterruptController, &UartPs,
//				   UART_DEVICE_ID, UART_INT_IRQ_ID);
//
//	xil_printf("--- INICIO ---\n");
//	while(1){
//		if(TotalReceivedCount)
//		{
//			TotalReceivedCount=0;
////			xil_printf("Datos: %s\n", RecvBuffer);
////			memset(RecvBuffer, 0, TEST_BUFFER_SIZE);
//		}
//	}
//	if (Status != XST_SUCCESS) {
//		xil_printf("UART Interrupt Example Test Failed\r\n");
//		return XST_FAILURE;
//	}
//
//	xil_printf("Successfully ran UART Interrupt Example Test\r\n");
//	return XST_SUCCESS;
//}


int UartPsIntrExample(INTC *IntcInstPtr, XUartPs *UartInstPtr, u16 DeviceId, u16 UartIntrId)
{
	int Status;
	XUartPs_Config *Config;
	u32 IntrMask;

	Config = XUartPs_LookupConfig(DeviceId);

	if (NULL == Config) {
		return XST_FAILURE;
	}

	Status = XUartPs_CfgInitialize(UartInstPtr, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Check hardware build */
	Status = XUartPs_SelfTest(UartInstPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the UART to the interrupt subsystem such that interrupts
	 * can occur. This function is application specific.
	 */
	Status = SetupInterruptSystem(IntcInstPtr, UartInstPtr, UartIntrId);


	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XUartPs_SetHandler(UartInstPtr, (XUartPs_Handler)Handler, UartInstPtr);

	IntrMask = XUARTPS_IXR_RXFULL |   // FIFO lleno
				XUARTPS_IXR_TOUT  |   // Timeout de recepción
				XUARTPS_IXR_RXOVR;    // Overflow en RX

	if (UartInstPtr->Platform == XPLAT_ZYNQ_ULTRA_MP) {
		IntrMask |= XUARTPS_IXR_RBRK;
	}

	XUartPs_SetInterruptMask(UartInstPtr, IntrMask);

	XUartPs_SetOperMode(UartInstPtr, XUARTPS_OPER_MODE_NORMAL);

	XUartPs_SetRecvTimeout(UartInstPtr,16);

	return XST_SUCCESS;
}

static int SetupInterruptSystem(INTC *IntcInstancePtr,
				XUartPs *UartInstancePtr,
				u16 UartIntrId)
{
	int Status;

	XScuGic_Config *IntcConfig; /* Config for interrupt controller */

	/* Initialize the interrupt controller driver */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
				       IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the interrupt controller interrupt handler to the
	 * hardware interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				     (Xil_ExceptionHandler) XScuGic_InterruptHandler,
				     IntcInstancePtr);

	/*
	 * Connect a device driver handler that will be called when an
	 * interrupt for the device occurs, the device driver handler
	 * performs the specific interrupt processing for the device
	 */
	Status = XScuGic_Connect(IntcInstancePtr, UartIntrId,
				 (Xil_ExceptionHandler) XUartPs_InterruptHandler,
				 (void *) UartInstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Enable the interrupt for the device */
	XScuGic_Enable(IntcInstancePtr, UartIntrId);

	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

void Handler(void *CallBackRef, u32 Event, unsigned int EventData)
{
	XUartPs *UartInstPtr = (XUartPs *) CallBackRef;

	/* All of the data has been sent */
	if (Event == XUARTPS_EVENT_SENT_DATA) {
		TotalSentCount = EventData;
	}

	/* All of the data has been received */
	if (Event == XUARTPS_EVENT_RECV_DATA ||
		Event == XUARTPS_EVENT_RECV_TOUT ||
		Event == XUARTPS_EVENT_RECV_ERROR ||
		Event == XUARTPS_EVENT_PARE_FRAME_BRKE ||
		Event == XUARTPS_EVENT_RECV_ORERR) {

		XUartPs_Recv(UartInstPtr, RecvBuffer, TEST_BUFFER_SIZE);

//		TotalReceivedCount = EventData;

		xil_printf("Datos: %s\n", RecvBuffer);
		memset(RecvBuffer, 0, TEST_BUFFER_SIZE);
	}

}

