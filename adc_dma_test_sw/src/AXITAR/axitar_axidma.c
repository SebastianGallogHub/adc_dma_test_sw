/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : axitar_axidma.c
 * Descripción         : Archivo de implementación para las herramientas de
 * 						 conexión con el IP AXI_DMA que obtiene los datos de
 * 						 AXI_TAR.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 08/11/2024
 * Fecha de modificación: 11/06/2025
 * Versión             : v1.0
 *
 * Institución         : Universidad Nacional de Rosario (UNR)
 * Carrera             : Ingeniería Electrónica
 *
 * Derechos reservados:
 * Este código ha sido desarrollado en el marco del Proyecto Final de Ingeniería
 * por Sebastián Nahuel Gallo. Su uso está autorizado únicamente por la
 * Comisión Nacional de Energía Atómica (CNEA) con fines internos.
 * Queda prohibida su reproducción, modificación o distribución sin
 * autorización expresa por escrito del autor.
 ***************************************************************/

/***************************** Include Files *******************************/
#include "../AXITAR/axitar_axidma.h"

#include "../includes/log.h"
#include "../includes/assert.h"
#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
void AXIDMA_RxIntrHandler(void *Callback);

/************************** Variable Definitions ***************************/

static XAxiDma AxiDma;

u8 stopDma = 0;
u16 bufferSectorSize = 0;
u16 bufferSectorCount = 0;
u16 bufferSectorToProcess = 0;
u64 bufferSectorsCompleted = 0; // Parte del ring buffer a procesar

u32 axiDmaIntCount = 0;
u32 axiDmaTransferCount = 0;
u32 Error = 0;

Intr_Config axiDmaIntrConfig;

int rbSize;
int bufferDataSize;
int wordsCounter = 0;


/****************************************************************************/

void AXIDMA_Reset() {
	int TimeOut = 10000;

	XAxiDma_Reset(&AxiDma);

	while (TimeOut--)
	{
		if (XAxiDma_ResetIsDone(&AxiDma))
			break;
	}

	Error = 0;
	stopDma = 0;
	bufferSectorSize = 0;
	bufferSectorCount = 0;
	bufferSectorToProcess = 0;
	bufferSectorsCompleted = 0; // Parte del ring buffer a procesar
	axiDmaIntCount = 0;
	axiDmaTransferCount = 0;
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

	// Configuro el handler de las interrupciones
	axiDmaIntrConfig.IntrId = AXIDMA_RX_INTR_ID;
	axiDmaIntrConfig.Handler = (Xil_ExceptionHandler)AXIDMA_RxIntrHandler;
	axiDmaIntrConfig.CallBackRef = RxRingPtr;
	axiDmaIntrConfig.Priority = 0xC0;

	IntrSystem_AddHandler(&axiDmaIntrConfig);

	stopDma = 0;

	return 0;
}

int AXIDMA_SetupRx(u32 ringBufferSize, u32 ringBufferSectorSize, u32 dataSize, int coalesceCount) {
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
    ASSERT(BdCount >= ringBufferSize, "No hay suficientes BDs para las transferencias. Hay %d pide %d", BdCount, ringBufferSize);

    // Creo el ringbuffer de BDs para la recepción, asignando el espacio de memoria definido
    status = XAxiDma_BdRingCreate(RxRingPtr, AXIDMA_RX_BD_SPACE_BASE, AXIDMA_RX_BD_SPACE_BASE,
                                  XAXIDMA_BD_MINIMUM_ALIGNMENT, ringBufferSize);
    ASSERT_SUCCESS(status, "Rx bd create failed");

    // Inicializo el ringbuffer de BDs con un template vacío
    XAxiDma_BdClear(&BdTemplate);
    ASSERT_SUCCESS(
    		XAxiDma_BdRingClone(RxRingPtr, &BdTemplate), "Rx bd clone failed");

    // Reservo la cantidad de BDs necesarios para las transferencias y obtiene el primer BD
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

    ASSERT_SUCCESS(
    		XAxiDma_BdRingSetCoalesce(RxRingPtr, coalesceCount, 255),
			"Rx set coalesce failed. Max 255 was %d", coalesceCount);

    // Paso el anillo de BDs configurado al hardware para que comience a usarse
    ASSERT_SUCCESS(
    		XAxiDma_BdRingToHw(RxRingPtr, ringBufferSize, BdPtr), "Rx ToHw failed");

    XAxiDma_BdRingIntEnable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);	// Habilito interrupciones del hardware

    XAxiDma_BdRingEnableCyclicDMA(RxRingPtr);					// Activo el modo cíclico
    XAxiDma_SelectCyclicMode(&AxiDma, XAXIDMA_DEVICE_TO_DMA, 1); // Configuro DMA para operar en modo cíclico

    // Iniciar DMA explícitamente
    status = XAxiDma_BdRingStart(RxRingPtr);
    ASSERT_SUCCESS(
    		status, "Error starting XAxiDma_BdRingStart");

    stopDma = 0;
    bufferSectorSize = ringBufferSectorSize;
    bufferSectorCount = ringBufferSize / ringBufferSectorSize;
    bufferSectorToProcess = 0;
    bufferSectorsCompleted = 0;

    rbSize = ringBufferSize;
    bufferDataSize = dataSize;
    wordsCounter = 0;

    return XST_SUCCESS;
}

void AXIDMA_StopRxAsync(){
	stopDma = 1;
}

void stopRx() {
	XAxiDma_BdRing *RxRingPtr = XAxiDma_GetRxRing(&AxiDma);

	// Deshabilitar interrupciones para evitar nuevas activaciones
	XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	// Desactivar modo cíclico directamente al registrar del DMA
	XAxiDma_SelectCyclicMode(&AxiDma, XAXIDMA_DEVICE_TO_DMA, 0);

	// Reset del DMA para garantizar que se limpien todos los estados internos
	XAxiDma_Reset(&AxiDma);

	// Esperar a que el reset finalice
	int TimeOut = 10000;
	while (TimeOut--) {
		if (XAxiDma_ResetIsDone(&AxiDma))
			break;
	}
}

int AXIDMA_BufferSectorComplete(u32 *bufferAddr){
	// Si en la interrupción aún no se completó ningún sector, salgo
	if (bufferSectorsCompleted <= 0)
		return 0;

	// Asigno la dirección del sector del buffer que corresponde
	*bufferAddr = (u32)(AXIDMA_RX_BUFFER_BASE + bufferSectorToProcess * bufferDataSize * bufferSectorSize);

	// Resto 1 a la cantidad de sectores de buffer completados
	bufferSectorsCompleted --;

	// Preparo el siguiente sector para ser procesado
	bufferSectorToProcess ++;

	// Verifico no sobrepasar la cantidad de sectores
	if (bufferSectorToProcess >= bufferSectorCount)
		bufferSectorToProcess = 0;

	return 1;
}

void AXIDMA_RxCallBack(XAxiDma_BdRing *RxRingPtr) {

	int BdCount;
	XAxiDma_Bd *BdPtr;

	BdCount = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	axiDmaTransferCount += BdCount;
	axiDmaIntCount ++;

	wordsCounter += BdCount;

	XAxiDma_BdRingFree(RxRingPtr, BdCount, BdPtr);

	if(wordsCounter >= bufferSectorSize){
		// Escribió un sector más
		bufferSectorsCompleted ++;
		if (bufferSectorsCompleted >= 0xFFFFFFFFFFFFFFFF) // Para que no se pase del máximo de u64
			bufferSectorsCompleted = 0;

		// Reseteo el contador de coalescencia para recaptar el suceso
		wordsCounter = 0;

		if(stopDma)
			stopRx();
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
