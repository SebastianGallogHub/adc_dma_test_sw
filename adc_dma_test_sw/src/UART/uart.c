/*
 * uart.c
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../UART/uart.h"

#include "xuartps.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "sleep.h"

#include "../UART/uart_dmaps.h"
#include "../UART/uart_mefCommand.h"
#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/
typedef enum{
	WAITING_COMMAND,
	COMMAND_RECEIVED,
	WAITING_PARAMETER_MSB,
	WAITING_PARAMETER_LSB,
}MEF_COMMAND_STATE;

/************************** Function Prototypes *****************************/
void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr);
void UART_Handler(void *CallBackRef, u32 Event, unsigned int EventData);
void UART_mefCommand(u8 chr);

/************************** Variable Definitions ***************************/
static XUartPs UartPs;

static u8 RecvBuffer[BUFFER_SIZE];	/* Buffer for Receiving Data */

// Envío de datos contínuos asincrónicos por DMA
volatile int uart0DoneTx = 0;
int maxData = 0;
int maxBytes = 0;
int pendingBytes = 0;
int sendBytes = 0;
u8 *nextBuffer;
int doneSendBuffer = 1;
u8 cancel = 0;

u32 IntrMask;

Intr_Config uartIntrConfig = {
		UART_INT_IRQ_ID,
		(Xil_ExceptionHandler)XUartPs_InterruptHandler_Wrapper,
		(void *)&UartPs,
		0x00 // Interrupción con prioridad más alta
};

/****************************************************************************/
void UART_Init() {
	XUartPs *UartPsPtr = &UartPs;
	XUartPs_Config *Config;

	Config = XUartPs_LookupConfig(UART_DEVICE_ID);

	XUartPs_ResetHw(Config->BaseAddress);

	usleep(1000000);

	XUartPs_CfgInitialize(UartPsPtr, Config, Config->BaseAddress);

	XUartPs_SetHandler(UartPsPtr, (XUartPs_Handler) UART_Handler, UartPsPtr);

	IntrMask =
		XUARTPS_IXR_TOUT | XUARTPS_IXR_PARITY  | XUARTPS_IXR_FRAMING |
		XUARTPS_IXR_OVER | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL  |
		XUARTPS_IXR_RXOVR;

	XUartPs_SetInterruptMask(UartPsPtr, IntrMask);

	XUartPs_SetRecvTimeout(UartPsPtr, 8);

	IntrSystem_AddHandler(&uartIntrConfig);

	DMAPS_Init();

	usleep(2000);
}

void UART_SetupRx(){
	// El buffer de recepción y su tamaño se configuran en esta función no bloqueante
	XUartPs_Recv(&UartPs, RecvBuffer, BUFFER_SIZE);
}

void UART_SendAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen) {

	// Para poder llamarla 2 veces seguidas
	while(1){
		if (UART_DoneTx())
			break;
	}

	// Delay necesario entre envíos sucesivos
	usleep(UART_MIN_DELAY_PER_BYTE_SEND * (buffSizeBytes/dataLen));

	DMAPS_ConfigSend(sendBufferAddr, (u32)UART_TX_RX_FIFO_ADDR, buffSizeBytes);

	uart0DoneTx = 0;
	DMAPS_Send();
}

void UART_SendBufferAsync_Cancel(){
	cancel = 1;
	maxData = 0;
	maxBytes = 0;
	sendBytes = 0;
	pendingBytes = 0;
	doneSendBuffer = 1;
}

void UART_SendBufferAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen, int cancelToken){
	if (cancelToken == 1) return;
	if (buffSizeBytes <= 0) return;
	if (sendBufferAddr == 0) return;

	cancel = cancelToken;

	// Configuro dinámicamente el límite la primera vez que llamo desde afuera
	if(dataLen != 0){
		doneSendBuffer = 0;
		maxBytes = UART_TX_FIFO_DEPTH - dataLen;
		maxData = maxBytes / dataLen;
	}

	// Determino cuántos bytes me quedan por enviar
	pendingBytes = buffSizeBytes;
	sendBytes = pendingBytes > maxBytes? maxBytes: pendingBytes;

	// Delay mandatorio
	usleep(UART_MIN_DELAY_PER_BYTE_SEND * maxData);

	DMAPS_ConfigSend(sendBufferAddr, (u32)UART_TX_RX_FIFO_ADDR, sendBytes);

	// Envío
	uart0DoneTx = 0;
	DMAPS_Send();

	// Recalculo el siguiente buffer
	pendingBytes -= sendBytes;
	if(pendingBytes > 0){
		nextBuffer = (u8*)(sendBufferAddr + sendBytes);
	}else{
		doneSendBuffer = 1;
	}
}

int UART_DoneTx(){
	int c = XUartPs_IsTransmitEmpty(&UartPs);
	return uart0DoneTx && c && DMAPS_Done();
}

int UART_DoneSendBuffer(){
	return doneSendBuffer && DMAPS_Done();
}

void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr){
	u32 IsrStatus;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	IsrStatus = XUartPs_ReadReg(InstancePtr->Config.BaseAddress,
				   XUARTPS_IMR_OFFSET);

	IsrStatus &= XUartPs_ReadReg(InstancePtr->Config.BaseAddress,
				   XUARTPS_ISR_OFFSET);

	if((IsrStatus & ((u32)XUARTPS_IXR_TXEMPTY | (u32)XUARTPS_IXR_TXFULL)) != (u32)0) {
		uart0DoneTx = 1;
		// Disparo una transacción por lo que quede, registrado en estas variables
		// 0 en data len para que se use el máximo dinámico de bytes a enviar en función
		// de la primera llamada
		UART_SendBufferAsync((u32)nextBuffer, pendingBytes, 0, cancel);
	}

	// Por último llamo a la función original que debía estar conectada al evento
	// para no romper con la cadena que ejecuta UART_Handler para la recepción
	XUartPs_InterruptHandler(InstancePtr);
}

void UART_Handler(void *CallBackRef, u32 Event, unsigned int EventData){
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
			UART_mefCommand(RecvBuffer[i]);
		}

		/*
		 * Para que EventData refleje el valor real de los bytes recibidos en la interrupción
		 * se debe relanzar la lectura. De este modo también se pisan los datos viejos de
		 * RecvBuffer. A su vez, la recepción por interrupción nunca termina porque
		 * ReceiveBufferPtr->RemainingBytes != 0 siempre
		 * */
		XUartPs_Recv(UartPsPtr, RecvBuffer, BUFFER_SIZE);
	}

//	if (Event == XUARTPS_EVENT_RECV_ERROR ||
//		Event == XUARTPS_EVENT_PARE_FRAME_BRKE ||
//		Event == XUARTPS_EVENT_RECV_ORERR) {}
}


