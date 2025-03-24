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
	int i;

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

	for (i = 0; i < BUFFER_SIZE-1; i++) {
		SendBuffer[i] = (i % 26) + 'A';
	}

	SendBuffer[i] = '\n'; // Agrego un enter para formato

	DMAPS_Init();
}

void UARTPS_0_Test() {
	UARTPS_0_SendAsync();

	while (1) {
			if ((TotalSentCount >= BUFFER_SIZE)
		&& (TotalReceivedCount >= BUFFER_SIZE)) {
			break;
		}
	}

	xil_printf("Recibido: (%d chr)\n%s \n", TotalReceivedCount, RecvBuffer);
	xil_printf("\nSuccessfully ran UART Interrupt Example Test\r\n");
}

int UARTPS_0_SendAsync() {
	XUartPs *UartPsPtr = &UartPs;

	/*
	 * El buffer de recepción y su tamaño se configuran en esta función no bloqueante
	 *
	 * A partir de ahora se registrará si una interrupción RX se da y
	 * se leen TODOS los bytes disponibles en el RX_FIFO y se guardan en buffer
	 *
	 * Una vez recibidos todos los datos se debería reiniciar la recepción con esta
	 * función (o modificando los datos del RXBuffer pero creo que esto es más
	 * seguro)
	 */
	XUartPs_Recv(UartPsPtr, RecvBuffer, BUFFER_SIZE);

	xil_printf("\r\nSeteando  DMA PARA ENVIAR POR UART\r\n");

	// Para que en Handler se reciba la cantidad de enviados a través de EventData
	UartPs.SendBuffer.RequestedBytes = BUFFER_SIZE;

	DMAPS_ConfigSend((u32)SendBuffer, (u32)UART_TX_RX_FIFO_ADDR,
			 1, 4, BUFFER_SIZE*sizeof(u8));

	// Espero a que se haya mandado todo lo disponible en el TX_FIFO
	while(!XUartPs_IsTransmitEmpty(UartPsPtr));

	DMAPS_Send();

	return 0;
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
		if(DMAPS_Done()){
			xil_printf("\r\nSuccessfully ran DMA Interrupt Example Test\r\n");
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
	XUartPsBuffer *ReceiveBufferPtr = &UartPsPtr->ReceiveBuffer;
	u32 bytesLeidos = 0;
	u32 IntrMask;

	if (Event == XUARTPS_EVENT_SENT_DATA) {
		TotalSentCount += EventData;

		// Reactivo la interrupción. Al parecer se desactiva en XUartPs_InterruptHandler
		IntrMask = XUartPs_GetInterruptMask(UartPsPtr);

		IntrMask |= XUARTPS_IXR_TXEMPTY;

		XUartPs_SetInterruptMask(UartPsPtr, IntrMask);
	}

	if (Event == XUARTPS_EVENT_RECV_DATA ||
		Event == XUARTPS_EVENT_RECV_TOUT) {

		bytesLeidos =
				ReceiveBufferPtr->RequestedBytes - ReceiveBufferPtr->RemainingBytes - TotalReceivedCount;

		TotalReceivedCount += bytesLeidos;

		// Para hacer un apéndice de lo que fue llegando, cambio la información del buffer
		// de RX de modo que el siguiente byte comience donde terminó el mensaje anterior.
		ReceiveBufferPtr->NextBytePtr = RecvBuffer + TotalReceivedCount;
	}

	if (Event == XUARTPS_EVENT_RECV_ERROR ||
		Event == XUARTPS_EVENT_PARE_FRAME_BRKE ||
		Event == XUARTPS_EVENT_RECV_ORERR) {

		TotalReceivedCount += EventData;
		TotalErrorCount++;

//		XUartPs_Recv((XUartPs*)CallBackRef, RecvBuffer, TEST_BUFFER_SIZE);
	}
}
