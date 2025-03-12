
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

#include "AXI_TAR.h"
#include "./Zmod/zmod.h"
#include "./ZmodADC1410/zmodadc1410.h"

#include "log.h"
#include "assert.h"

//-----------------------------------------------------------------------------
// ZMOD Parameters
#define ZMOD_BASE_ADDR  	XPAR_AXI_ZMODADC1410_0_S00_AXI_BASEADDR
#define ZMOD_ADC_INTR_ID  	XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR // Es el mismo que el de DMA porque no está cableado
#define IIC_BASE_ADDR		XPAR_PS7_I2C_1_BASEADDR

//-----------------------------------------------------------------------------
// DMA Parameters
#define DMA_DEV_ID				XPAR_AXI_DMA_0_DEVICE_ID
#define DMA_BASE_ADDR  			XPAR_AXI_DMA_0_BASEADDR
#define DMA_NUMBER_OF_TRANSFERS 200

//-----------------------------------------------------------------------------
// TAR Parameters
#define TAR_BASE			XPAR_AXI_TAR_0_S00_AXI_BASEADDR
// General config
#define TAR_CONFIG_OFF		AXI_TAR_S00_AXI_SLV_REG0_OFFSET
#define TAR_StopAll()		AXI_TAR_mWriteReg(TAR_BASE,TAR_CONFIG_OFF, 0x00);
// TAR
#define TAR_CH1_HIST_OFF	AXI_TAR_S00_AXI_SLV_REG1_OFFSET
#define TAR_CH2_HIST_OFF	AXI_TAR_S00_AXI_SLV_REG2_OFFSET
#define TAR_Start()			AXI_TAR_mWriteReg(TAR_BASE,TAR_CONFIG_OFF, 0x01);
// master_test
#define master_COUNT_CFG_OFF	AXI_TAR_S00_AXI_SLV_REG1_OFFSET
#define master_COUNT_OFF		AXI_TAR_S00_AXI_SLV_REG3_OFFSET
#define master_INTR_COUNT_OFF	AXI_TAR_S00_AXI_SLV_REG2_OFFSET
#define TAR_Start_master_test()	AXI_TAR_mWriteReg(TAR_BASE,TAR_CONFIG_OFF, 0x10);

//#define TAR_TRANSFER_COUNT 	1000000//10ms
#define TAR_TRANSFER_COUNT 	10000000//100ms

//-----------------------------------------------------------------------------
// Memoria
#define MEM_BASE_ADDR			(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000)

#define DMA_RX_BD_SPACE_BASE	(MEM_BASE_ADDR)
#define DMA_RX_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0000FFFF)
#define DMA_RX_BD_SPACE 		DMA_RX_BD_SPACE_HIGH - DMA_RX_BD_SPACE_BASE + 1
#define DMA_RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x000300000)
#define DMA_RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x0004FFFFF)

//-----------------------------------------------------------------------------
//Interupciones
#define INTC			XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler

#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define DMA_RX_INTR_ID		XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR
#define TAR_DR_INTR_ID		XPAR_FABRIC_AXI_TAR_0_INTROUT_INTR

//-----------------------------------------------------------------------------
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

u32 dmaIntCount = 0;
u32 dmaTransferCount = 0;
u32 tarTransferCount = 0;
u32 Error = 0;

XAxiDma AxiDma;
static INTC Intc;
XTime tStart, tFinish, tElapsed;
#define GetElapsed_ns  ((tFinish-tStart)*3)
#define GetElapsed_us  GetElapsed_ns/1000
#define GetElapsed_ms  GetElapsed_ns/1000000

//-----------------------------------------------------------------------------
int main()
{
	int Status;
	int TimeOut = 1000;
	u32 slv_reg0, slv_reg1, slv_reg2, slv_reg3;

//	CLEAR_SCREEN;

	LOG(0, "--------------------- INICIO MAIN -----------------------");
	LOG(1, "PRUEBA SOLO DE LAS INTERRUPCIONES DE MASTER_TEST");

	for (int i = 0; i<=10; i++)
	{

		LOG(0, "--------------------- (%d) -----------------------", i);
		ASSERT_SUCCESS(
				SetupIntrSystem(&Intc, &AxiDma, DMA_RX_INTR_ID, TAR_DR_INTR_ID),
				"Fallo al inicializar el sistema de interrupciones");

		ASSERT_SUCCESS(
				DMA_Init(&AxiDma, DMA_NUMBER_OF_TRANSFERS, sizeof(u32)),
				"Fallo al inicializar el DMA.");

	//	ZMOD_Init();

		TAR_Init(TAR_TRANSFER_COUNT);

		LOG(1, "----- Inicio interrupciones");
		TAR_Start_master_test();
		XTime_GetTime(&tStart);

		Status = Xil_WaitForEventSet(1000000U, 1, &Error);
		if (Status == XST_SUCCESS) {
			LOG(0, "--> Receive error %d", Status);
			goto goOut;
		}

		while(tarTransferCount <= DMA_NUMBER_OF_TRANSFERS){
			if(!(TimeOut--))
			{
				TimeOut = 1000;
				//Confirmo si está prendido
				slv_reg0 = AXI_TAR_mReadReg(TAR_BASE, TAR_CONFIG_OFF);
				//Valor de la cuenta de muestras configurado
				slv_reg1 = AXI_TAR_mReadReg(TAR_BASE, master_COUNT_CFG_OFF);
				//Valor de la cuenta de muestras actual
				slv_reg3 = AXI_TAR_mReadReg(TAR_BASE, master_COUNT_OFF);
				//Valor de la cuenta de interrupciones lanzadas != capturadas por core
				slv_reg2 = AXI_TAR_mReadReg(TAR_BASE, master_INTR_COUNT_OFF);
			}
		}

		TAR_StopAll();
		XAxiDma_Reset(&AxiDma);

		TimeOut = 10000;
		while (TimeOut)
		{
			if (XAxiDma_ResetIsDone(&AxiDma))
				break;
			TimeOut--;
		}

		DisableIntrSystem(&Intc, DMA_RX_INTR_ID, TAR_DR_INTR_ID);

		//Imprimir todos los valores recibidos junto a su dirección
		PrintRxData();
		dmaIntCount = 0;
		dmaTransferCount = 0;
		tarTransferCount = 0;
	}

	LOG(0, "--------------------- FIN MAIN -----------------------");
	return 0;

goOut:
	TAR_StopAll();
	XAxiDma_Reset(&AxiDma);
	LOG(0, "--------------------- FIN MAIN -----------------------");
	return XST_FAILURE;
}

void PrintRxData()
{
	u32* buffer  = (u32*)DMA_RX_BUFFER_BASE;
	LOG_LINE;LOG_LINE;
	LOG(1, "Datos recibidos");

	LOG(2, "Interrupciones recibidas por DMA: %d", dmaIntCount);
	LOG(2, "Transferencias recibidas por DMA: %d", dmaTransferCount);
	LOG(2, "Transferencias lanzadas por TAR: %d", tarTransferCount);
	LOG(2, "--------------------------------------");
	LOG(2, "\t\t\tCH1 \t|\tCH2", tarTransferCount);
	for (u32 i = 0; i<= dmaTransferCount; i++){
		LOG(2, "%d)\t%d  \t|\t%d\t\t[0x%08x]", i,  (u16)((buffer[i] >> 16) & 0xffff), (u16)(buffer[i] & 0xffFF), &buffer[i]);
	}

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

	//Inicializar el controlador de interrupciones
	ASSERT_SUCCESS(
			XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress),
			"Falla al inicializar las configuraciones de INTC");

	//Interrupción TAR
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, TarIntrId, 0xA0, 0x3);
	ASSERT_SUCCESS(
			XScuGic_Connect(IntcInstancePtr, TarIntrId, (Xil_InterruptHandler)TAR_IntrHandler, (void *)TAR_BASE),
			"Failed to connect interruption handler TAR_IntrHandler");
	XScuGic_Enable(IntcInstancePtr, TarIntrId);

	//Interrupción DMA
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, DmaRxIntrId, 0xA0, 0x3);
	ASSERT_SUCCESS(
			XScuGic_Connect(IntcInstancePtr, DmaRxIntrId, (Xil_InterruptHandler)DMA_RxIntrHandler, RxRingPtr),
			"Failed to connect interruption handler DMA_RxIntrHandler");
	XScuGic_Enable(IntcInstancePtr, DmaRxIntrId);

	//Interrupción de hardware
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
	 * El resto de las comunicaciones se realizarán mediante AXI_TAR + AXI_DMA
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

int DMA_Init(XAxiDma* AxiDma, u32 cntTransferencias, u32 dataLen)
{
    LOG(1, "DMA_Init");

    XAxiDma_Config *Config;
    XAxiDma_BdRing *RxRingPtr;
    XAxiDma_Bd BdTemplate, *BdPtr, *BdCurPtr;
    UINTPTR RxBufferPtr;
    u32 i, BdCount;
    u32 coalesceCounter = 10;
    int status;

    // Obtengo la configuración del DMA usando un identificador predefinido
    Config = XAxiDma_LookupConfig(DMA_DEV_ID);
    ASSERT(Config != NULL, "No config found for %d", DMA_DEV_ID);

    // Inicializo la instancia del DMA con la configuración obtenida.
    ASSERT_SUCCESS(
    		XAxiDma_CfgInitialize(AxiDma, Config), "DMA initialization failed");

    // Verifico que el dispositivo esté configurado en modo Scatter-Gather (SG)
    // y no en modo simple.
    ASSERT(
    		XAxiDma_HasSg(AxiDma), "Device configured as Simple mode");

    RxRingPtr = XAxiDma_GetRxRing(AxiDma);

    // Deshabilito todas las interrupciones del anillo para evitar activaciones indeseadas durante la configuración
    XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

    // Reinicio del DMA para evitar estados incorrectos
    XAxiDma_Reset(AxiDma);
    while (!XAxiDma_ResetIsDone(AxiDma));

    // Calculo la cantidad de BDs disponibles en el espacio asignado para la recepción
    BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, DMA_RX_BD_SPACE);
    ASSERT(
    		BdCount >= cntTransferencias, "No hay suficientes BDs para las transferencias");

    // Creo el ringbuffer de BDs para la recepción, asignando el espacio de memoria definido
    status = XAxiDma_BdRingCreate(RxRingPtr, DMA_RX_BD_SPACE_BASE, DMA_RX_BD_SPACE_BASE,
                                  XAXIDMA_BD_MINIMUM_ALIGNMENT, cntTransferencias);
    ASSERT_SUCCESS(
    		status, "Rx bd create failed");

    // Inicializo el ringbuffer de BDs con un template vacío
    XAxiDma_BdClear(&BdTemplate);
    ASSERT_SUCCESS(
    		XAxiDma_BdRingClone(RxRingPtr, &BdTemplate), "Rx bd clone failed");

    // Reservo la cantidad de BDs necesarios para las transferencias y obtiene el primer BD
    ASSERT_SUCCESS(
    		XAxiDma_BdRingAlloc(RxRingPtr, cntTransferencias, &BdPtr), "Rx bd alloc failed");

    BdCurPtr = BdPtr;
    RxBufferPtr = DMA_RX_BUFFER_BASE;

    // Alinear el buffer a 64 bits para cumplir requisitos de hardware
    RxBufferPtr = (RxBufferPtr + 63) & ~0x3F;

    // Configuro cada BD para las transferencias de recepción:
    for (i = 0; i < cntTransferencias; i++) {
    	// Asigno la dirección del buffer para el BD actual
        status = XAxiDma_BdSetBufAddr(BdCurPtr, RxBufferPtr);
    	ASSERT_SUCCESS(
    			status, "Rx set buffer addr %x on BD %x failed %d\r\n",(unsigned int)RxBufferPtr,(UINTPTR)BdCurPtr);

    	// Configuro la longitud de datos a transferir, respetando el máximo permitido
    	status = XAxiDma_BdSetLength(BdCurPtr, dataLen, RxRingPtr->MaxTransferLen);
    	ASSERT_SUCCESS(
    			status, "Rx set length %d on BD %x failed %d\r\n", dataLen, (UINTPTR)BdCurPtr);

    	// Asigno un identificador al BD basado en la dirección del buffer, para su seguimiento posterior
        XAxiDma_BdSetCtrl(BdCurPtr, 0);
        XAxiDma_BdSetId(BdCurPtr, RxBufferPtr);

        // Actualizo el puntero del buffer para la siguiente transferencia y avanzo al siguiente BD del ringbuffer
        RxBufferPtr += dataLen;
        BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RxRingPtr, BdCurPtr);
    }

    // Limpieza del buffer de llegada
    memset((void*)DMA_RX_BUFFER_BASE, 0, DMA_RX_BUFFER_HIGH - DMA_RX_BUFFER_BASE);

    // Me aseguro que las actualizaciones en la memoria se reflejen correctamente
    // realizando un flush de la cache para los BDs y el buffer de recepción
    Xil_DCacheFlushRange((UINTPTR)BdPtr, cntTransferencias * sizeof(XAxiDma_Bd));
    Xil_DCacheFlushRange((UINTPTR)DMA_RX_BUFFER_BASE, cntTransferencias * dataLen);

    ASSERT_SUCCESS(
    		XAxiDma_BdRingSetCoalesce(RxRingPtr, coalesceCounter, 100), "Rx set coalesce failed");

    // Paso el anillo de BDs configurado al hardware para que comience a usarse
    ASSERT_SUCCESS(
    		XAxiDma_BdRingToHw(RxRingPtr, cntTransferencias, BdPtr), "Rx ToHw failed");

    XAxiDma_BdRingIntEnable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);	// Habilito interrupciones
    XAxiDma_BdRingEnableCyclicDMA(RxRingPtr);					// Activo el modo cíclico
    XAxiDma_SelectCyclicMode(AxiDma, XAXIDMA_DEVICE_TO_DMA, 1); // Configuro DMA para operar en modo cíclico

    // Iniciar DMA explícitamente
    status = XAxiDma_BdRingStart(RxRingPtr);
    ASSERT_SUCCESS(
    		status, "Error starting XAxiDma_BdRingStart");

    LOG(2, "DMA configurado para recibir %d transferencias. Longitud %d bytes", cntTransferencias, dataLen);
    LOG(2, "Interrupciones cada %d transacciones", coalesceCounter);
    LOG(2, "Modo cíclico activado");
    LOG(2, "Primer buffer de datos: direccion 0x%08x",  (unsigned int)XAxiDma_BdGetId(BdPtr));

    return XST_SUCCESS;
}
void DMA_RxCallBack(XAxiDma_BdRing *RxRingPtr)
{
	// Cuento las transferencias hechas para finalizar el bucle principal
	// Debería hacerse una interrupción cada transferencia

//	if (dmaTransferCount >= DMA_NUMBER_OF_TRANSFERS)
//		return;

	int BdCount;
	XAxiDma_Bd *BdPtr;

	BdCount = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	dmaTransferCount += BdCount;
	dmaIntCount ++;

	XAxiDma_BdRingFree(RxRingPtr, BdCount, BdPtr);

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

	TAR_StopAll();
	do{/*Espero a que se registre el valor de stop*/}
	while(AXI_TAR_mReadReg(TAR_BASE,TAR_CONFIG_OFF));

	AXI_TAR_mWriteReg(TAR_BASE, master_COUNT_CFG_OFF, cuenta);

	if (cuenta/100 > 1000) {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d ms)", cuenta, cuenta/100000);
	} else {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d us)", cuenta, cuenta/100);
	}

	return;
}
void TAR_IntrHandler(void * Callback)
{
	if(tarTransferCount <= UINT32_MAX)
	{
		tarTransferCount ++;
//		XTime_GetTime(&tFinish);
//		LOG(2, "%d) %d ms ", tarTransferCount, GetElapsed_ms);
	}

	return;
}














