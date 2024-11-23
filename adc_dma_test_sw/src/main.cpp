
#include <stdio.h>
#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xdebug.h"
#include "xil_util.h"
#include "xil_io.h"
#include "xtime_l.h"
#include "xscugic.h"

#include <stdlib.h>
#include <unistd.h>
#include "xil_printf.h"
#include "xparameters.h"

#include "./TAR/TAR.h"
#include "./Zmod/zmod.h"
#include "./ZmodADC1410/zmodadc1410.h"

#include "log.h"
#include "assert.h"

// ZMOD Parameters
#define ZMOD_BASE_ADDR  	XPAR_AXI_ZMODADC1410_0_S00_AXI_BASEADDR
#define ZMOD_ADC_INTR_ID  	XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR // Es el mismo que el de DMA porque no está cableado
#define IIC_BASE_ADDR		XPAR_PS7_I2C_1_BASEADDR

// DMA Parameters
#define DMA_DEV_ID				XPAR_AXI_DMA_0_DEVICE_ID
#define DMA_BASE_ADDR  			XPAR_AXI_DMA_0_BASEADDR
#define DMA_NUMBER_OF_TRANSFERS 100

// TAR Parameters
#define TAR_BASE			XPAR_TAR_0_S00_AXI_BASEADDR
#define TAR_START_OFF		TAR_S00_AXI_SLV_REG0_OFFSET
#define TAR_COUNT_CFG_OFF	TAR_S00_AXI_SLV_REG1_OFFSET
#define TAR_Start()			TAR_mWriteReg(TAR_BASE,TAR_START_OFF, 1);
#define TAR_Stop()			TAR_mWriteReg(TAR_BASE,TAR_START_OFF, 0);

// Memoria
#define MEM_BASE_ADDR			(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000)

#define DMA_RX_BD_SPACE_BASE	(MEM_BASE_ADDR)
#define DMA_RX_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0000FFFF)
#define DMA_RX_BD_SPACE 		DMA_RX_BD_SPACE_HIGH - DMA_RX_BD_SPACE_BASE + 1
#define DMA_RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00010000)
#define DMA_RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x0002FFFF)

//Interupciones
#define INTC			XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler

#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define DMA_RX_INTR_ID		XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR
#define TAR_DR_INTR_ID		XPAR_FABRIC_TAR_0_INTROUT_INTR

//Funciones
void ZMOD_Init();

int DMA_Init(XAxiDma*, u32 cnt, u32 len);
int DMA_SG_Cyclic_Rx_Init(u32 cnt, u32 len);
void DMA_RxCallBack(XAxiDma_BdRing *RxRingPtr);
void DMA_RxIntrHandler(void *Callback);

void TAR_Init(u32);
void TAR_IntrHandler(void *Callback);

void PrintRxData();

int SetupIntrSystem(INTC*, XAxiDma*, u16 DmaRxIntrId, u16 TARIntrId);
void DisableIntrSystem(INTC*, u16 DmaRxIntrId, u16 TARIntrId);

u32 transferencias = 0;
u32 transferenciasLanzadas = 0;
u32 Error = 0;

XAxiDma AxiDma;
static INTC Intc;

int main()
{
	int Status;

	CLEAR_SCREEN;

	LOG(0, "--------------------- INICIO MAIN -----------------------");

	ZMOD_Init();

	ASSERT_SUCCESS(
			DMA_Init(&AxiDma, DMA_NUMBER_OF_TRANSFERS, sizeof(u32)),
			"Fallo al inicializar el DMA.");

	TAR_Init(10000);

	ASSERT_SUCCESS(
			SetupIntrSystem(&Intc, &AxiDma, DMA_RX_INTR_ID, TAR_DR_INTR_ID),
			"Fallo al inicializar el sistema de interrupciones");

	TAR_Start();

	Status = Xil_WaitForEventSet(1000000U, 1, &Error);
	if (Status == XST_SUCCESS) {
		LOG(1, "Receive error %d", Status);
		return XST_FAILURE;
	}

	while(transferencias <= DMA_NUMBER_OF_TRANSFERS){	}

	TAR_Stop();
	XAxiDma_Reset(&AxiDma);

	int TimeOut = 10000;
	while (TimeOut)
	{
		if (XAxiDma_ResetIsDone(&AxiDma))
			break;
		TimeOut--;
	}

	//Imprimir todos los valores recibidos junto a su dirección
	PrintRxData();

	DisableIntrSystem(&Intc, DMA_RX_INTR_ID, TAR_DR_INTR_ID);

	LOG(0, "--------------------- FIN MAIN -----------------------");

	return 0;
}

void PrintRxData()
{
	LOG_LINE;LOG_LINE;
	LOG(1, "Datos recibidos");

	LOG(2, "Transferencias recibidas por DMA: %d", transferencias);
	LOG(2, "Transferencias lanzadas por TAR: %d", transferenciasLanzadas);

	//TODO imprimir los datos por uart

	return;
}

void DisableIntrSystem(INTC *IntcInstancePtr, u16 RxIntrId, u16 TarIntrId)
{
	XScuGic_Disconnect(IntcInstancePtr, TarIntrId);
	XScuGic_Disconnect(IntcInstancePtr, RxIntrId);
}
int SetupIntrSystem(INTC *IntcInstancePtr, XAxiDma *AxiDmaPtr, u16 DmaRxIntrId, u16 TarIntrId)
{
	LOG(1, "SetupIntrSystem");

	XAxiDma_BdRing *RxRingPtr = XAxiDma_GetRxRing(AxiDmaPtr);
	XScuGic_Config *IntcConfig;

	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	ASSERT_SUCCESS(
			XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress),
			"Falla al inicializar las configuraciones de INTC");

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, TarIntrId, 0xA0, 0x3);

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, DmaRxIntrId, 0xA0, 0x3);

	ASSERT_SUCCESS(
			XScuGic_Connect(IntcInstancePtr, TarIntrId, (Xil_InterruptHandler)TAR_IntrHandler, (void *)TAR_BASE),
			"Failed to connect interruption handler TAR_IntrHandler");

	ASSERT_SUCCESS(
			XScuGic_Connect(IntcInstancePtr, DmaRxIntrId, (Xil_InterruptHandler)DMA_RxIntrHandler, RxRingPtr),
			"Failed to connect interruption handler DMA_RxIntrHandler");

	XScuGic_Enable(IntcInstancePtr, TarIntrId);
	XScuGic_Enable(IntcInstancePtr, DmaRxIntrId);

	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(
			XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler)INTC_HANDLER,
			(void *)IntcInstancePtr);

	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

void ZMOD_Init()
{
	LOG(1, "ZMOD_Init");
	/*
	 * Esta función utiliza la clase ZMODADC1410 para comunicarse con
	 * ZMOD1410 AXI ADAPTER y configurar el ZMOD140 a través del
	 * LLC. Este es su único propósito.
	 *
	 * El resto de las comunicaciones se realizarán mediante TAR+DMA
	 * */

	// Crear el objeto ZMODADC1410 para configurar la ganancia y acoplamiento
	ZMODADC1410 adcZmod(ZMOD_BASE_ADDR,
						DMA_BASE_ADDR,    //Este parámetro se configura pero no se usa
						IIC_BASE_ADDR,
						MEM_BASE_ADDR,    //Este parámetro se configura pero no se usa
						ZMOD_ADC_INTR_ID, //Este parámetro se configura pero no se usa
						DMA_RX_INTR_ID);  //Este parámetro se configura pero no se usa

	//Canal 1
	LOG(2, "CH1: LOW Gain; DC Coupling");
	adcZmod.setGain(0, 0);
	adcZmod.setCoupling(0, 0); //0 for DC Coupling, 1 for AC Coupling

	//Canal 2
	LOG(2, "CH2: LOW Gain; DC Coupling");
	adcZmod.setGain(1, 0);
	adcZmod.setCoupling(1, 0); //0 for DC Coupling, 1 for AC Coupling

	return;
}

int DMA_Init(XAxiDma* AxiDma, u32 cntTransferencias, u32 transLen)
{
	LOG(1, "DMA_Init");

	XAxiDma_Config *Config;
	XAxiDma_BdRing *RxRingPtr;
	XAxiDma_Bd BdTemplate,*BdPtr, *BdCurPtr;
	UINTPTR RxBufferPtr;
	u32 i, BdCount;

	Config = XAxiDma_LookupConfig(DMA_DEV_ID);
	if (!Config) {
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);
		return XST_FAILURE;
	}

	XAxiDma_CfgInitialize(AxiDma, Config);

	ASSERT(
			XAxiDma_HasSg(AxiDma),
			"Device configured as Simple mode");

	// TODO Jugar entre poner un bd para cada dato y poner un solo BD para todos los datos
		// esto último hay que ver como se juega con tlast

	// TODO Imprimir las direcciones de DB y la de los buffers

	RxRingPtr = XAxiDma_GetRxRing(AxiDma);

	/* Disable all RX interrupts before RxBD space setup */
	XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Setup Rx BD space */
	BdCount = (u32)XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, DMA_RX_BD_SPACE);

	ASSERT(
			BdCount >= cntTransferencias,
			"No es posible alojar todas las transferencias en el espacio disponible "
			"0x%x - 0x%x (%d)",
			DMA_RX_BD_SPACE_BASE,
			DMA_RX_BD_SPACE_HIGH,
			DMA_RX_BD_SPACE);

	ASSERT_SUCCESS(
			XAxiDma_BdRingCreate(RxRingPtr,
							     DMA_RX_BD_SPACE_BASE,
								 DMA_RX_BD_SPACE_BASE,
								 XAXIDMA_BD_MINIMUM_ALIGNMENT,
								 cntTransferencias),
			"Rx bd create failed");

	/* Setup a BD template for the Rx channel. Then copy it to every RX BD */
	XAxiDma_BdClear(&BdTemplate);

	ASSERT_SUCCESS(
			XAxiDma_BdRingClone(RxRingPtr, &BdTemplate),
			"Rx bd clone failed");

	/* Attach buffers to RxBD ring so we are ready to receive packets */
	ASSERT_SUCCESS(
			XAxiDma_BdRingAlloc(RxRingPtr, cntTransferencias, &BdPtr),
			"Rx bd alloc failed with %d\r\n");

	BdCurPtr = BdPtr;
	RxBufferPtr = DMA_RX_BUFFER_BASE;

//	LOG(2, "Se configuran %d Bds para Rx", FreeBdCount);
//	LOG(2,"Bd configurados en memoria");
//	LOG(3, "Desde: 0x%X\tHasta: 0x%X (%d de bds)\tLimite: 0x%X",
//			(u8*)DMA_RX_BD_SPACE_BASE,
//			(u8*)DMA_RX_BD_SPACE_BASE+FreeBdCount*bdSize,
//			FreeBdCount,
//			(u8*)RX_BD_SPACE_HIGH);
//	LOG(2,"Destino de los datos en memoria");
//	LOG(3, "Desde: 0x%X\tHasta: 0x%X (%d de slots de %db de largo = %d KB)",
//			(u8*)RX_BUFFER_BASE,
//			(u8*)(RX_BUFFER_BASE+MAX_PKT_LEN*FreeBdCount),
//			FreeBdCount,
//			MAX_PKT_LEN,
//			B2KB(FreeBdCount*MAX_PKT_LEN));
	for (i = 0; i <  cntTransferencias; i++) {

		ASSERT_SUCCESS(
				XAxiDma_BdSetBufAddr(BdCurPtr, RxBufferPtr),
				"Rx set buffer addr %x on BD %x failed",
				(unsigned int)RxBufferPtr,
				(UINTPTR)BdCurPtr);

		ASSERT_SUCCESS(
				XAxiDma_BdSetLength(BdCurPtr, transLen, RxRingPtr->MaxTransferLen),
				"Rx set length %d on BD %x failed %d\r\n",
				transLen,
				(UINTPTR)BdCurPtr);

		/* Receive BDs do not need to set anything for the control
		 * The hardware will set the SOF/EOF bits per stream status
		 */
		XAxiDma_BdSetCtrl(BdCurPtr, 0);

		XAxiDma_BdSetId(BdCurPtr, RxBufferPtr);

		RxBufferPtr += transLen;
		BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RxRingPtr, BdCurPtr);
	}

	//Limpiando el buffer de llegada
	RxBufferPtr = DMA_RX_BUFFER_BASE;	i = 0;
//	LOG(3,"Rx @ DIR");
	do
	{
		*((u8*)RxBufferPtr) = 0;

//		if(i<=10)
//		{
//			LOG(3, "0x%04X @ 0x%X", *((u8*)RxBufferPtr), RxBufferPtr);
//		}
//
//		if((u8*)RxBufferPtr==(u8*)(DMA_RX_BUFFER_BASE+transLen*cntTransferencias))
//		{
//			LOG(3,"...");
//			LOG(3, "0x%04X @ 0x%X**", *((u8*)RxBufferPtr), RxBufferPtr);
//		}

//		i ++;
		RxBufferPtr += sizeof(u8);
	}while(RxBufferPtr <= DMA_RX_BUFFER_HIGH);

	LOG(2, "Buffer limpio");
//	RxBufferPtr -= sizeof(u8);
//	LOG(3, "...");
//	LOG(3, "0x%04X @ 0x%X", *((u8*)RxBufferPtr), RxBufferPtr);

	ASSERT_SUCCESS(
			XAxiDma_BdRingSetCoalesce(RxRingPtr, 1, 100),
			"Rx set coalesce failed");

	ASSERT_SUCCESS(
			XAxiDma_BdRingToHw(RxRingPtr, cntTransferencias, BdPtr),
			"Rx ToHw failed");

	/* Enable all RX interrupts */
	XAxiDma_BdRingIntEnable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Enable Cyclic DMA mode */
	XAxiDma_BdRingEnableCyclicDMA(RxRingPtr);
	XAxiDma_SelectCyclicMode(AxiDma, XAXIDMA_DEVICE_TO_DMA, 1);

	/* Start RX DMA channel */
	ASSERT_SUCCESS(
			XAxiDma_BdRingStart(RxRingPtr),
			"Rx start BD ring failed");

	LOG(2, "DMA configurado para recibir %d transferencias", cntTransferencias);

	return XST_SUCCESS;
}
void DMA_RxCallBack(XAxiDma_BdRing *RxRingPtr)
{
	// Cuento las transferencias hechas para finalizar el bucle principal
	// Debería hacerse una interrupción cada transferencia

	if (transferencias >= DMA_NUMBER_OF_TRANSFERS)
		return;

	int BdCount;
	XAxiDma_Bd *BdPtr;

	BdCount = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	transferencias += BdCount;

	return;
}
void DMA_RxIntrHandler(void *Callback)
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

		//Esto imprime valores importantes de los registros de RxRingPtr
		// por consola
		XAxiDma_BdRingDumpRegs(RxRingPtr);

		Error = 1;

		/* Reset could fail and hang
		 * NEED a way to handle this or do not call it??
		 */
		XAxiDma_Reset(&AxiDma);

		TimeOut = 10000;

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
	if ((IrqStatus & (XAXIDMA_IRQ_DELAY_MASK | XAXIDMA_IRQ_IOC_MASK)))
		DMA_RxCallBack(RxRingPtr);

	return;
}

void TAR_Init(u32 cuenta)
{
	LOG(1, "TAR_Init");

	TAR_Stop();
	do{/*Espero a que se registre el valor de stop*/}
	while(TAR_mReadReg(TAR_BASE,TAR_START_OFF));

	TAR_mWriteReg(TAR_BASE, TAR_COUNT_CFG_OFF, cuenta);

	LOG(2,"TAR configurado para interrumpir cada %d muestras", cuenta);

	return;
}
void TAR_IntrHandler(void * Callback)
{
	if(transferenciasLanzadas <= UINT32_MAX)
		transferenciasLanzadas ++;
	return;
}














