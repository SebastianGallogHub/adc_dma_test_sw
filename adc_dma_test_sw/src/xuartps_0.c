/*
 * xuartps_0.c
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/

#include "xuartps.h"

#include "xil_exception.h"
#include "xil_printf.h"

#include "xuartps_0.h"
#include "xuartps_0_xdmaps.h"
#include "interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr);
void UART_O_Handler(void *CallBackRef, u32 Event, unsigned int EventData);

/************************** Variable Definitions ***************************/

static XUartPs UartPs;

static u8 SendBuffer[BUFFER_SIZE];	/* Buffer for Transmitting Data */
static u8 RecvBuffer[BUFFER_SIZE];	/* Buffer for Receiving Data */

volatile int receivedCommand = 0;
volatile int sendOnRepeat = 0;

volatile int TotalReceivedCount;
volatile int TotalSentCount;
int TotalErrorCount;

Intr_Config uartIntrConfig = {
		UART_INT_IRQ_ID,
		(Xil_ExceptionHandler)XUartPs_InterruptHandler_Wrapper,
		(void *)&UartPs,
		0x70 // Interrupción con prioridad más alta
};

/****************************************************************************/

void UARTPS_0_Init() {
	XUartPs *UartPsPtr = &UartPs;
	XUartPs_Config *Config;
	u32 IntrMask;

	Config = XUartPs_LookupConfig(UART_DEVICE_ID);

	XUartPs_CfgInitialize(UartPsPtr, Config, Config->BaseAddress);

	XUartPs_SetHandler(UartPsPtr, (XUartPs_Handler) UART_O_Handler, UartPsPtr);

	IntrMask =
		XUARTPS_IXR_TOUT | XUARTPS_IXR_PARITY  | XUARTPS_IXR_FRAMING |
		XUARTPS_IXR_OVER | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL  |
		XUARTPS_IXR_RXOVR;

	XUartPs_SetInterruptMask(UartPsPtr, IntrMask);

	XUartPs_SetRecvTimeout(UartPsPtr, 8);

	AddIntrHandler(&uartIntrConfig);

	DMAPS_Init();
}

void UARTPS_0_Test() {
	int i;
	for (i = 0; i < BUFFER_SIZE-1; i++) {
		SendBuffer[i] = (i % 26) + 'A';
	}

	SendBuffer[i] = '\n'; // Agrego un enter para formato

	UARTPS_0_ConfigSendAsync((u32)SendBuffer, BUFFER_SIZE * sizeof(u8));

	while (1) {
		if(receivedCommand)
		{
			if(receivedCommand == 1)
			{
				sendOnRepeat = 1;
				UARTPS_0_SendAsync();
			}else{
				sendOnRepeat = 0;
			}

			receivedCommand = 0;
		}
	}
}

int UARTPS_0_ConfigSendAsync(u32 sendBufferAddr, int buffSizeBytes) {
	/*
	 * El buffer de recepción y su tamaño se configuran en esta función no bloqueante
	 */
	XUartPs_Recv(&UartPs, RecvBuffer, BUFFER_SIZE);

//	xil_printf("Seteando  DMA PARA ENVIAR POR UART\r\n");

	// Para que en Handler se reciba la cantidad de enviados a través de EventData
	UartPs.SendBuffer.RequestedBytes = BUFFER_SIZE;

	DMAPS_ConfigSend(sendBufferAddr, (u32)UART_TX_RX_FIFO_ADDR,
			 1, 4, buffSizeBytes);

	return 0;
}

void UARTPS_0_SendAsync() {
	// Espero a que se haya mandado todo lo disponible en el TX_FIFO
	while(!XUartPs_IsTransmitEmpty(&UartPs));
	DMAPS_Send();
}

void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr){
	u32 IsrStatus;
	int i=0;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	IsrStatus = XUartPs_ReadReg(InstancePtr->Config.BaseAddress,
				   XUARTPS_IMR_OFFSET);

	IsrStatus &= XUartPs_ReadReg(InstancePtr->Config.BaseAddress,
				   XUARTPS_ISR_OFFSET);

	if((IsrStatus & ((u32)XUARTPS_IXR_TXEMPTY)) != (u32)0) {
		// Sé que puedo enviar este mensaje por acá porque TXEMPTY
		if(DMAPS_Done() && sendOnRepeat){
			DMAPS_Send();
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

void UART_O_Handler(void *CallBackRef, u32 Event, unsigned int EventData)
{
	XUartPs *UartPsPtr = (XUartPs*)CallBackRef;
	u32 IntrMask;

	if (Event == XUARTPS_EVENT_SENT_DATA) {
		// Reactivo la interrupción. Al parecer se desactiva en XUartPs_InterruptHandler
		IntrMask = XUartPs_GetInterruptMask(UartPsPtr);

		IntrMask |= XUARTPS_IXR_TXEMPTY;

		XUartPs_SetInterruptMask(UartPsPtr, IntrMask);
	}

	if (Event == XUARTPS_EVENT_RECV_DATA ||
		Event == XUARTPS_EVENT_RECV_TOUT) {

		for (unsigned int i = 0; i < EventData; i++) {
			if(RecvBuffer[i] == 'a'){
				receivedCommand = 1;
				break;
			}
			if(RecvBuffer[i] == 'f'){
				receivedCommand = 2;
				break;
			}
		}

		/*
		 * Para que EventData refleje el valor real de los bytes recibidos en la interrupción
		 * se debe relanzar la lectura. De este modo también se pisan los datos viejos de
		 * RecvBuffer. A su vez, la recepción por interrupción nunca termina porque
		 * ReceiveBufferPtr->RemainingBytes != 0 siempre
		 * */
		XUartPs_Recv(&UartPs, RecvBuffer, BUFFER_SIZE);
	}

	if (Event == XUARTPS_EVENT_RECV_ERROR ||
		Event == XUARTPS_EVENT_PARE_FRAME_BRKE ||
		Event == XUARTPS_EVENT_RECV_ORERR) {

//		TotalReceivedCount += EventData;
//		TotalErrorCount++;

//		XUartPs_Recv((XUartPs*)CallBackRef, RecvBuffer, TEST_BUFFER_SIZE);
	}
}
