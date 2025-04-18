/*
 * dma_config.h
 *
 *  Created on: Nov 8, 2024
 *      Author: sebas
 */

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

typedef void (*AXIDMA_ProcessBufferDelegate) (unsigned char *sendBuffer, unsigned int countSectors);


int AXIDMA_Init();

void AXIDMA_Reset();

int AXIDMA_SetupRx(u32 ringBufferSize, u32 dataSize, int bufferProcessCoalesce, AXIDMA_ProcessBufferDelegate processBuffer);

#endif /* SRC_AXITAR_AXITAR_AXIDMA_H_ */
