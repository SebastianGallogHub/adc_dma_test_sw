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
#include "sleep.h"

#include "xuartps_0.h"
#include "xuartps_0_xdmaps.h"

#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr);
void UART_O_Handler(void *CallBackRef, u32 Event, unsigned int EventData);
void setIntrTXEMPTY_On();
void clearIntrTXEMPTY();

/************************** Variable Definitions ***************************/

static XUartPs UartPs;

//static u8 SendBuffer[BUFFER_SIZE];	/* Buffer for Transmitting Data */
static u8 RecvBuffer[BUFFER_SIZE];	/* Buffer for Receiving Data */

volatile int receivedCommand = 0;
volatile int sendOnRepeat = 0;
int uart0DoneTx = 0;

volatile int TotalReceivedCount;
volatile int TotalSentCount;
int TotalErrorCount;
u32 IntrMask;

Intr_Config uartIntrConfig = {
		UART_INT_IRQ_ID,
		(Xil_ExceptionHandler)XUartPs_InterruptHandler_Wrapper,
		(void *)&UartPs,
		0x00 // Interrupción con prioridad más alta
};

/****************************************************************************/

void UARTPS_0_Init() {
	XUartPs *UartPsPtr = &UartPs;
	XUartPs_Config *Config;

	Config = XUartPs_LookupConfig(UART_DEVICE_ID);

	XUartPs_ResetHw(Config->BaseAddress);

	usleep(1000000);

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

void UARTPS_0_StartRx(){
	/*
	 * El buffer de recepción y su tamaño se configuran en esta función no bloqueante
	 */
	XUartPs_Recv(&UartPs, RecvBuffer, BUFFER_SIZE);
}

void UARTPS_0_SendAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen) {

	// Para poder llamarla 2 veces seguidas
	while(1){
		if (UARTPS_0_DoneTx())
			break;
	}

	// Delay necesario entre envíos sucesivos
	usleep(UART_MIN_DELAY_PER_BYTE_SEND * (buffSizeBytes/dataLen));

	DMAPS_ConfigSend(sendBufferAddr, (u32)UART_TX_RX_FIFO_ADDR, buffSizeBytes);

	uart0DoneTx = 0;
	DMAPS_Send();
}

int maxData = 0;
int maxBytes = 0;
int pendingBytes = 0;
int sendBytes = 0;
u8 *nextBuffer;
int doneSendBuffer = 1;

void UARTPS_0_SendBufferAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen){
	if (buffSizeBytes == 0) return;
	if (sendBufferAddr == 0) return;

	// Configuro dinámicamente el límite la primera vez que llamo y hago la copia
	if(dataLen != 0){
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
	if(pendingBytes != 0){
		doneSendBuffer = 0;
		nextBuffer = (u8*)(sendBufferAddr + sendBytes);
	}else{
		doneSendBuffer = 1;
	}
}

int UARTPS_0_DoneTx(){
	int c = XUartPs_IsTransmitEmpty(&UartPs);
	return uart0DoneTx && c && DMAPS_Done();
}

int UARTPS_0_DoneSendBuffer(){
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
		UARTPS_0_SendBufferAsync((u32)nextBuffer, pendingBytes, 0);
	}

	// Por último llamo a la función original que debía estar conectada al evento
	// para no romper con la cadena que ejecuta UART_Handler para la recepción
	XUartPs_InterruptHandler(InstancePtr);
}

u8 command = 0;
u8 parameter_f = 0;
u16 parameter = 0;

UART_COMMAND UART_0_GetCommand(){
	u8 c = command;
	command = 0;
	return (UART_COMMAND)c;
}

u8 UART_0_HasParameter(){
	return parameter_f;
}

u16 UART_0_GetParameter(){
	if (!parameter_f) return 0;
	u16 p = parameter;
	parameter = 0;
	parameter_f = 0;
	return p;
}

void mefCommand(u8 chr){
	static int state = 0;

	switch (state) {
	case 0:
		if(chr == COMMAND_FORMAT_HEADER)
			state = 1;

		break;

	case 1:
		if(chr == CMD_START ||
		   chr == CMD_STOP){
			command = chr;
			state = 0;
		}
		if(chr == CMD_CHA_H_L ||
		   chr == CMD_CHA_H_H ||
		   chr == CMD_CHB_H_L ||
		   chr == CMD_CHB_H_H){
			command = chr;
			parameter = (u16)chr << 8;
			state = 2;
		}
		break;

	case 2:
		parameter_f = 1;
		parameter |= chr;
		state = 0;
		break;

	default:
			break;
	}
}

void UART_O_Handler(void *CallBackRef, u32 Event, unsigned int EventData){
	static int state = 0;
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
			mefCommand(RecvBuffer[i]);
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
