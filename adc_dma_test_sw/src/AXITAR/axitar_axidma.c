/*
 * dma_config.c
 *
 *  Created on: Nov 8, 2024
 *      Author: sebas
 */

/***************************** Include Files *******************************/

#include "../AXITAR/axitar_axidma.h"

#include "../includes/assert.h"
#include "../includes/log.h"
#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void AXIDMA_RxIntrHandler(void *Callback);

/************************** Variable Definitions ***************************/

static XAxiDma AxiDma;

u32 axiDmaIntCount = 0;
u32 axiDmaTransferCount = 0;
u32 Error = 0;

Intr_Config axiDmaIntrConfig;

int rbSize;
int bufferDataSize;
int bufferProcessCoalesceCounter = 0;
AXIDMA_ProcessBufferDelegate ProcessBufferDelegate;


/****************************************************************************/

void AXIDMA_Reset() {
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

int AXIDMA_Init() {
	XAxiDma_Config *Config;
	XAxiDma_BdRing *RxRingPtr;

	// Obtengo la configuración del DMA usando un identificador predefinido
	Config = XAxiDma_LookupConfig(AXIDMA_DEV_ID);
	ASSERT(Config != NULL, "No config found for %d", AXIDMA_DEV_ID);

	// Inicializo la instancia del DMA con la configuración obtenida.
	ASSERT_SUCCESS(
			XAxiDma_CfgInitialize(&AxiDma, Config), "DMA initialization failed");

	// Cargo la configuración de interrupciones
	RxRingPtr = XAxiDma_GetRxRing(&AxiDma);

	//dmaIntrConfig
	axiDmaIntrConfig.IntrId = AXIDMA_RX_INTR_ID;
	axiDmaIntrConfig.Handler = (Xil_ExceptionHandler)AXIDMA_RxIntrHandler;
	axiDmaIntrConfig.CallBackRef = RxRingPtr;
	axiDmaIntrConfig.Priority = 0xC0;

	IntrSystem_AddHandler(&axiDmaIntrConfig);

	return 0;
}

int AXIDMA_SetupRx(u32 ringBufferSize, u32 dataSize, int coalesceCount, AXIDMA_ProcessBufferDelegate processBuffer) {
    LOG(1, "AXI_DMA_Init");

    XAxiDma_BdRing *RxRingPtr;
    XAxiDma_Bd BdTemplate, *BdPtr, *BdCurPtr;
    UINTPTR RxBufferPtr;
    u32 BdCount;
    int status;

    /* Verifico que el dispositivo esté configurado en modo Scatter-Gather (SG)
       y no en modo simple. */
    ASSERT(XAxiDma_HasSg(&AxiDma), "Device configured as Simple mode");

    RxRingPtr = XAxiDma_GetRxRing(&AxiDma);

    // Deshabilito todas las interrupciones del anillo para evitar activaciones indeseadas durante la configuración
    XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

    // Reinicio del DMA para evitar estados incorrectos
    AXIDMA_Reset();

    // Calculo la cantidad de BDs disponibles en el espacio asignado para la recepción
    BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, AXIDMA_RX_BD_SPACE);
    ASSERT(BdCount >= ringBufferSize, "No hay suficientes BDs para las transferencias");

    // Creo el ringbuffer de BDs para la recepción, asignando el espacio de memoria definido
    status = XAxiDma_BdRingCreate(RxRingPtr, AXIDMA_RX_BD_SPACE_BASE, AXIDMA_RX_BD_SPACE_BASE,
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
    RxBufferPtr = AXIDMA_RX_BUFFER_BASE;

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
    memset((void*)AXIDMA_RX_BUFFER_BASE, 0, AXIDMA_RX_BUFFER_HIGH - AXIDMA_RX_BUFFER_BASE);

    /* Me aseguro que las actualizaciones en la memoria se reflejen correctamente
       realizando un flush de la cache para los BDs y el buffer de recepción */
    Xil_DCacheFlushRange((UINTPTR)BdPtr, ringBufferSize * sizeof(XAxiDma_Bd));
    Xil_DCacheFlushRange((UINTPTR)AXIDMA_RX_BUFFER_BASE, ringBufferSize * dataSize);

    LOG(2, "Interrupciones cada %d transacciones", coalesceCount);
    ASSERT_SUCCESS(
    		XAxiDma_BdRingSetCoalesce(RxRingPtr, coalesceCount, 255),
			"Rx set coalesce failed. Max 255 was %d", coalesceCount);

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
    ProcessBufferDelegate = processBuffer;

    return XST_SUCCESS;
}

void AXIDMA_RxCallBack(XAxiDma_BdRing *RxRingPtr) {

	int BdCount;
	XAxiDma_Bd *BdPtr;

	BdCount = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	axiDmaTransferCount += BdCount;
	axiDmaIntCount ++;

	bufferProcessCoalesceCounter += BdCount;

	XAxiDma_BdRingFree(RxRingPtr, BdCount, BdPtr);

	// Coalescencia manual porque no la está haciendo bien
	if((ProcessBufferDelegate != NULL) && (bufferProcessCoalesceCounter >= rbSize)){

		// Proceso el buffer con el handler
		ProcessBufferDelegate((unsigned char*)AXIDMA_RX_BUFFER_BASE, 2);

		// Reseteo el contador de coalescencia para recaptar el suceso
		bufferProcessCoalesceCounter = 0;
	}

	return;
}

void AXIDMA_RxIntrHandler(void *Callback) {
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
		AXIDMA_RxCallBack(RxRingPtr);

	return;
}
