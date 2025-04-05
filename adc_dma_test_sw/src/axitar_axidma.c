/*
 * dma_config.c
 *
 *  Created on: Nov 8, 2024
 *      Author: sebas
 */

/***************************** Include Files *******************************/

#include "axitar_axidma.h"

#include "interruptSystem.h"

#include "assert.h"
#include "log.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void AXI_DMA_RxIntrHandler(void *Callback);

/************************** Variable Definitions ***************************/

static XAxiDma AxiDma;

u32 axiDmaIntCount = 0;
u32 axiDmaTransferCount = 0;
u32 Error = 0;

Intr_Config axiDmaIntrConfig;

int bufferDataSize;
int rbSize;
int dataCoalesce;
AXI_DMA_ProcessBufferDelegate ProcessBuffer;


/****************************************************************************/

void AXI_DMA_Reset() {
	int TimeOut = 10000;

	axiDmaIntCount = 0;
	axiDmaTransferCount = 0;
	Error = 0;

	XAxiDma_Reset(&AxiDma);

	while (TimeOut--)
	{
		if (XAxiDma_ResetIsDone(&AxiDma))
			break;
	}
}

int AXI_DMA_Init() {
	XAxiDma_Config *Config;
	XAxiDma_BdRing *RxRingPtr;

	// Obtengo la configuración del DMA usando un identificador predefinido
	Config = XAxiDma_LookupConfig(AXI_DMA_DEV_ID);
	ASSERT(Config != NULL, "No config found for %d", AXI_DMA_DEV_ID);

	// Inicializo la instancia del DMA con la configuración obtenida.
	ASSERT_SUCCESS(
			XAxiDma_CfgInitialize(&AxiDma, Config), "DMA initialization failed");

	// Cargo la configuración de interrupciones
	RxRingPtr = XAxiDma_GetRxRing(&AxiDma);

	//dmaIntrConfig
	axiDmaIntrConfig.IntrId = AXI_DMA_RX_INTR_ID;
	axiDmaIntrConfig.Handler = (Xil_ExceptionHandler)AXI_DMA_RxIntrHandler;
	axiDmaIntrConfig.CallBackRef = RxRingPtr;
	axiDmaIntrConfig.Priority = 0xA0;

	AddIntrHandler(&axiDmaIntrConfig);

	return 0;
}

int AXI_DMA_SetupRx(u32 ringBufferSize, u32 dataSize, int maxCntDataSend, AXI_DMA_ProcessBufferDelegate processBuffer) {
    LOG(1, "AXI_DMA_Init");

    XAxiDma_BdRing *RxRingPtr;
    XAxiDma_Bd BdTemplate, *BdPtr, *BdCurPtr;
    UINTPTR RxBufferPtr;
    u32 BdCount;
    int status;

    // Verifico que el dispositivo esté configurado en modo Scatter-Gather (SG)
    // y no en modo simple.
    ASSERT(XAxiDma_HasSg(&AxiDma), "Device configured as Simple mode");

    RxRingPtr = XAxiDma_GetRxRing(&AxiDma);

    // Deshabilito todas las interrupciones del anillo para evitar activaciones indeseadas durante la configuración
    XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

    // Reinicio del DMA para evitar estados incorrectos
    AXI_DMA_Reset();

    // Calculo la cantidad de BDs disponibles en el espacio asignado para la recepción
    BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, AXI_DMA_RX_BD_SPACE);
    ASSERT(BdCount >= ringBufferSize, "No hay suficientes BDs para las transferencias");

    // Creo el ringbuffer de BDs para la recepción, asignando el espacio de memoria definido
    status = XAxiDma_BdRingCreate(RxRingPtr, AXI_DMA_RX_BD_SPACE_BASE, AXI_DMA_RX_BD_SPACE_BASE,
                                  XAXIDMA_BD_MINIMUM_ALIGNMENT, ringBufferSize);
    ASSERT_SUCCESS(status, "Rx bd create failed");

    // Inicializo el ringbuffer de BDs con un template vacío
    XAxiDma_BdClear(&BdTemplate);
    ASSERT_SUCCESS(
    		XAxiDma_BdRingClone(RxRingPtr, &BdTemplate), "Rx bd clone failed");

    // Reservo la cantidad de BDs necesarios para las transferencias y obtiene el primer BD
    LOG(2, "DMA configurado para recibir %d transferencias. Longitud %d bytes", ringBufferSize, dataSize);
    ASSERT_SUCCESS(
    		XAxiDma_BdRingAlloc(RxRingPtr, ringBufferSize, &BdPtr), "Rx bd alloc failed");

    BdCurPtr = BdPtr;
    RxBufferPtr = AXI_DMA_RX_BUFFER_BASE;

    // Alinear el buffer a 64 bits para cumplir requisitos de hardware
    RxBufferPtr = (RxBufferPtr + 63) & ~0x3F;

    // Configuro cada BD para las transferencias de recepción:
    for (u32 i = 0; i < ringBufferSize; i++) {
    	// Asigno la dirección del buffer para el BD actual
        status = XAxiDma_BdSetBufAddr(BdCurPtr, RxBufferPtr);
    	ASSERT_SUCCESS(
    			status, "Rx set buffer addr %x on BD %x failed %d\r\n",(unsigned int)RxBufferPtr,(UINTPTR)BdCurPtr);

    	// Configuro la longitud de datos a transferir, respetando el máximo permitido
    	status = XAxiDma_BdSetLength(BdCurPtr, dataSize, RxRingPtr->MaxTransferLen);
    	ASSERT_SUCCESS(
    			status, "Rx set length %d on BD %x failed %d\r\n", dataSize, (UINTPTR)BdCurPtr);

    	// Asigno un identificador al BD basado en la dirección del buffer, para su seguimiento posterior
        XAxiDma_BdSetCtrl(BdCurPtr, 0);
        XAxiDma_BdSetId(BdCurPtr, RxBufferPtr);

        // Actualizo el puntero del buffer para la siguiente transferencia y avanzo al siguiente BD del ringbuffer
        RxBufferPtr += dataSize;
        BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RxRingPtr, BdCurPtr);
    }

    // Limpieza del buffer de llegada
    memset((void*)AXI_DMA_RX_BUFFER_BASE, 0, AXI_DMA_RX_BUFFER_HIGH - AXI_DMA_RX_BUFFER_BASE);

    // Me aseguro que las actualizaciones en la memoria se reflejen correctamente
    // realizando un flush de la cache para los BDs y el buffer de recepción
    Xil_DCacheFlushRange((UINTPTR)BdPtr, ringBufferSize * sizeof(XAxiDma_Bd));
    Xil_DCacheFlushRange((UINTPTR)AXI_DMA_RX_BUFFER_BASE, ringBufferSize * dataSize);

    LOG(2, "Interrupciones cada %d transacciones", AXI_DMA_COALESCE);
    ASSERT_SUCCESS(
    		XAxiDma_BdRingSetCoalesce(RxRingPtr, AXI_DMA_COALESCE, 100), "Rx set coalesce failed");

    // Paso el anillo de BDs configurado al hardware para que comience a usarse
    ASSERT_SUCCESS(
    		XAxiDma_BdRingToHw(RxRingPtr, ringBufferSize, BdPtr), "Rx ToHw failed");

    XAxiDma_BdRingIntEnable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);	// Habilito interrupciones

    LOG(2, "Modo cíclico activado");
    XAxiDma_BdRingEnableCyclicDMA(RxRingPtr);					// Activo el modo cíclico
    XAxiDma_SelectCyclicMode(&AxiDma, XAXIDMA_DEVICE_TO_DMA, 1); // Configuro DMA para operar en modo cíclico

    // Iniciar DMA explícitamente
    status = XAxiDma_BdRingStart(RxRingPtr);
    ASSERT_SUCCESS(
    		status, "Error starting XAxiDma_BdRingStart");

    LOG(2, "Primer buffer de datos: direccion 0x%08x",  (unsigned int)XAxiDma_BdGetBufAddr(BdPtr));

    rbSize = ringBufferSize;
    bufferDataSize = dataSize;
    dataCoalesce = maxCntDataSend;
    ProcessBuffer = processBuffer;

    return XST_SUCCESS;
}

u8 *sendBufferAddr = 0;
int coalesceCounter = 0;

#define bufferOverflow(sendBufferAddr, coalesceCounter) \
	((u32)sendBufferAddr + coalesceCounter * bufferDataSize) - (AXI_DMA_RX_BUFFER_BASE + rbSize * bufferDataSize)

void AXI_DMA_RxCallBack(XAxiDma_BdRing *RxRingPtr) {
	// Cuento las transferencias hechas para finalizar el bucle principal
	// Debería hacerse una interrupción cada transferencia

//	if (axiDmaTransferCount >= AXI_DMA_NUMBER_OF_TRANSFERS)
//		return;

	int BdCount;
	int overflow;
	XAxiDma_Bd *BdPtr;

	BdCount = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	axiDmaTransferCount += BdCount;
	axiDmaIntCount ++;

	coalesceCounter += BdCount;
	if(!sendBufferAddr)
		sendBufferAddr = (u8*)XAxiDma_BdGetBufAddr(BdPtr);

	XAxiDma_BdRingFree(RxRingPtr, BdCount, BdPtr);

	// Coalescencia manual porque no la está haciendo bien
	if((ProcessBuffer != NULL) && (coalesceCounter >= dataCoalesce)){

		overflow = bufferOverflow(sendBufferAddr, coalesceCounter);

		if(overflow > 0){
			// Enviar datos hasta el final del buffer
			ProcessBuffer((u32)sendBufferAddr, ((coalesceCounter * bufferDataSize) - overflow), bufferDataSize);

			// Enviar datos restantes desde el principio
			sendBufferAddr = (u8*)AXI_DMA_RX_BUFFER_BASE;
			ProcessBuffer((u32)sendBufferAddr, overflow, bufferDataSize);

		}else{
			ProcessBuffer((u32)sendBufferAddr, coalesceCounter * bufferDataSize, bufferDataSize);
		}

		coalesceCounter = 0;
		sendBufferAddr = 0;
	}

	return;
}

void AXI_DMA_RxIntrHandler(void *Callback) {
	XAxiDma_BdRing *RxRingPtr = (XAxiDma_BdRing *) Callback;
	u32 IrqStatus;
	int TimeOut;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_BdRingGetIrq(RxRingPtr);

	/* Acknowledge pending interrupts */
	XAxiDma_BdRingAckIrq(RxRingPtr, IrqStatus);

	// If no interrupt is asserted, we do not do anything
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
		AXI_DMA_RxCallBack(RxRingPtr);

	return;
}
