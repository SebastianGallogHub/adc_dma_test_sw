/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : axitar_axidma.h
 * Descripción         : Definiciones de constantes y funciones para la comunicación
 * 						 con IP AXI_DMA que transfiere a memoria los datos provenientes
 * 						 de IP AXI_TAR.
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

#ifndef SRC_AXITAR_AXITAR_AXIDMA_H_
#define SRC_AXITAR_AXITAR_AXIDMA_H_

#include "xparameters.h"
#include "xaxidma.h"

#define AXIDMA_DEV_ID				XPAR_AXI_DMA_0_DEVICE_ID
#define AXIDMA_BASE_ADDR  			XPAR_AXI_DMA_0_BASEADDR
//#define AXI_DMA_NUMBER_OF_TRANSFERS 128
//#define AXI_DMA_COALESCE			AXI_DMA_NUMBER_OF_TRANSFERS/8

#define MEM_BASE_ADDR				(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000)

// Buffer descriptor
#define AXIDMA_RX_BD_SPACE_BASE	(MEM_BASE_ADDR)
#define AXIDMA_RX_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0002FFFFF)
#define AXIDMA_RX_BD_SPACE 		AXIDMA_RX_BD_SPACE_HIGH - AXIDMA_RX_BD_SPACE_BASE + 1

// Buffer de datos de llegada
#define AXIDMA_RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x000300000)
#define AXIDMA_RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x0004FFFFF)
#define AXIDMA_RX_BUFFER_SPACE		AXIDMA_RX_BUFFER_HIGH - AXIDMA_RX_BUFFER_BASE + 1

#define AXIDMA_RX_INTR_ID			XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR

extern u32 axiDmaIntCount;
extern u32 axiDmaTransferCount;
extern u32 Error;

int AXIDMA_Init();
int AXIDMA_SetupRx(u32 ringBufferSize, u32 ringBufferSectorSize, u32 dataSize, int bufferProcessCoalesce);
void AXIDMA_StopRxAsync();
int AXIDMA_BufferSectorComplete(u32 *bufferAddr);

#endif /* SRC_AXITAR_AXITAR_AXIDMA_H_ */
