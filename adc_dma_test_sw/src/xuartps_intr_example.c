/***************************** Include Files *******************************/

#include "xparameters.h"

#include "xdmaps.h"
#include "xuartps.h"
#include "xscugic.h"

#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xil_assert.h"
#include "xil_io.h"
#include "xil_cache.h"

#include "interrupt_config.h"

/************************** Constant Definitions **************************/

#define UART_DEVICE_ID		XPAR_XUARTPS_0_DEVICE_ID
#define UART_INT_IRQ_ID		XPAR_XUARTPS_0_INTR
#define UART_TX_RX_FIFO_ADDR	XPAR_XUARTPS_0_BASEADDR + XUARTPS_FIFO_OFFSET

#define INTC				XScuGic
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID

#define TEST_BUFFER_SIZE	40

#define DMA_DEVICE_ID 		XPAR_XDMAPS_1_DEVICE_ID
#define DMA_DONE_INTR_0		XPAR_XDMAPS_0_DONE_INTR_0
#define DMA_FAULT_INTR		XPAR_XDMAPS_0_FAULT_INTR
#define DMA_LENGTH			1024	/* Length of the Dma Transfers */
#define TIMEOUT_LIMIT 		0x2000	/* Loop count for timeout */

/**************************** Type Definitions ******************************/


/************************** Function Prototypes *****************************/

void DMA_Init2();
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);

void UART_Init();
int DMA_UART_Send(int send_normal);
void XUartPs_InterruptHandler_Wrapper(XUartPs *InstancePtr);
void Handler(void *CallBackRef, u32 Event, unsigned int EventData);


/************************** Variable Definitions ***************************/

XDmaPs DmaInstance;
XUartPs UartPs	;		/* Instance of the UART Device */
INTC InterruptController;	/* Instance of the Interrupt Controller */

static u8 SendBuffer[TEST_BUFFER_SIZE];	/* Buffer for Transmitting Data */
static u8 RecvBuffer[TEST_BUFFER_SIZE];	/* Buffer for Receiving Data */

volatile int TotalReceivedCount;
volatile int TotalSentCount;
int TotalErrorCount;

unsigned int Channel = 0;
XDmaPs_Cmd DmaCmd2;
volatile int Checked;

Intr_Config dmaFaultIntrConfig = {
		DMA_FAULT_INTR,
		(Xil_ExceptionHandler)XDmaPs_FaultISR,
		(void *)&DmaInstance,
		0xA0
};

Intr_Config dmaCh0IntrConfig = {
		DMA_DONE_INTR_0,
		(Xil_ExceptionHandler)XDmaPs_DoneISR_0,
		(void *)&DmaInstance,
		0xA0
};

Intr_Config uartIntrConfig = {
		UART_INT_IRQ_ID,
		(Xil_ExceptionHandler)XUartPs_InterruptHandler_Wrapper,
		(void *)&UartPs,
		0x70 // Interrupción con prioridad más alta
};

int main (void){

	DMA_Init2();

	UART_Init();

	SetupIntrSystem();

	return DMA_UART_Send(0);
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void DMA_Init2(){
	XDmaPs_Config *DmaCfg;

	DmaCfg = XDmaPs_LookupConfig(DMA_DEVICE_ID);

	XDmaPs_CfgInitialize(&DmaInstance, DmaCfg, DmaCfg->BaseAddress);

	Xil_ExceptionInit();

	XDmaPs_SetDoneHandler(&DmaInstance, Channel, DmaDoneHandler,(void *)&Checked);

	AddIntrHandler(&dmaFaultIntrConfig);
	AddIntrHandler(&dmaCh0IntrConfig);

}
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef){
	volatile int *Checked = (volatile int *)CallbackRef;

	*Checked = 1;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void UART_Init() {
	XUartPs_Config* Config;
	u32 IntrMask;

	Config = XUartPs_LookupConfig(UART_DEVICE_ID);

	XUartPs_CfgInitialize(&UartPs, Config, Config->BaseAddress);

	XUartPs_SetHandler(&UartPs, (XUartPs_Handler) Handler, &UartPs);

	IntrMask =
		XUARTPS_IXR_TOUT | XUARTPS_IXR_PARITY  | XUARTPS_IXR_FRAMING |
		XUARTPS_IXR_OVER | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL  |
		XUARTPS_IXR_RXOVR;

	XUartPs_SetInterruptMask(&UartPs, IntrMask);

	XUartPs_SetRecvTimeout(&UartPs, 8);

	AddIntrHandler(&uartIntrConfig);
}
int DMA_UART_Send(int send_normal) {
	int Index;

	for (Index = 0; Index < TEST_BUFFER_SIZE; Index++) {
		SendBuffer[Index] = (Index % 26) + 'A';
		RecvBuffer[Index] = 0;
	}

	/*
	 * El buffer y su tamaño se configuran en esta función no bloqueante
	 *
	 * A partir de ahora se registrará si una interrupción RX se da y
	 * se leen TODOS los bytes disponibles en el RX_FIFO y se guardan en buffer
	 *
	 * Una vez recibidos todos los datos se debería reiniciar la recepción con esta
	 * función (o modificando los datos del RXBuffer pero creo que esto es más
	 * seguro)
	 */
	XUartPs_Recv(&UartPs, RecvBuffer, TEST_BUFFER_SIZE);

	if (send_normal) {
		// Esto enviaría los datos directamente byte a byte por UART
		XUartPs_Send(&UartPs, SendBuffer, TEST_BUFFER_SIZE);
	}
	else {

		xil_printf("\r\nSeteando  DMA PARA ENVIAR POR UART\r\n");

		// Para que en Handler se reciba la cantidad de enviados a través de EventData
		UartPs.SendBuffer.RequestedBytes = TEST_BUFFER_SIZE;

		// Confiuro el control de DMA para enviar datos desde el buffer Src al TX_FIFO de UART
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

		// Espero a que se haya mandado todo lo disponible en el TX_FIFO
		while(!XUartPs_IsTransmitEmpty(&UartPs));

		XDmaPs_Start(&DmaInstance, Channel, &DmaCmd2, 0);
	}

	while (1) {
		if ((TotalSentCount >= TEST_BUFFER_SIZE)
		&& (TotalReceivedCount >= TEST_BUFFER_SIZE)) {
			break;
		}
	}

	xil_printf("Recibido: (%d chr) %s \n", TotalReceivedCount, RecvBuffer);
	xil_printf("\nSuccessfully ran UART Interrupt Example Test\r\n");

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
void Handler(void *CallBackRef, u32 Event, unsigned int EventData)
{
	XUartPs *UartInst = (XUartPs*)CallBackRef;
	XUartPsBuffer *bufferPtr = &UartInst->ReceiveBuffer;
	u32 bytesLeidos = 0;
	u32 IntrMask;

	/* All of the data has been sent */
	if (Event == XUARTPS_EVENT_SENT_DATA) {
		TotalSentCount += EventData;

		// Reactivo la interrupción. Al parecer se desactiva en XUartPs_InterruptHandler
		IntrMask = XUartPs_GetInterruptMask(UartInst);

		IntrMask |= XUARTPS_IXR_TXEMPTY;

		XUartPs_SetInterruptMask(UartInst, IntrMask);
	}

	/* All of the data has been received */
	if (Event == XUARTPS_EVENT_RECV_DATA ||
		Event == XUARTPS_EVENT_RECV_TOUT) {

		bytesLeidos =
				bufferPtr->RequestedBytes - bufferPtr->RemainingBytes - TotalReceivedCount;

		TotalReceivedCount += bytesLeidos;

		// Para hacer un apéndice de lo que fue llegando, cambio la información del buffer
		// de RX de modo que el siguiente byte comience donde terminó el mensaje anterior.
		bufferPtr->NextBytePtr = RecvBuffer + TotalReceivedCount;
	}

	if (Event == XUARTPS_EVENT_RECV_ERROR ||
		Event == XUARTPS_EVENT_PARE_FRAME_BRKE ||
		Event == XUARTPS_EVENT_RECV_ORERR) {

		TotalReceivedCount += EventData;
		TotalErrorCount++;

//		XUartPs_Recv((XUartPs*)CallBackRef, RecvBuffer, TEST_BUFFER_SIZE);
	}
}
