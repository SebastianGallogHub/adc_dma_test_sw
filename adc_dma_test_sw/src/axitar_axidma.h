/*
 * dma_config.h
 *
 *  Created on: Nov 8, 2024
 *      Author: sebas
 */

#ifndef SRC_AXITAR_AXIDMA_H_
#define SRC_AXITAR_AXIDMA_H_

#include "xparameters.h"
#include "xaxidma.h"

#define AXI_DMA_DEV_ID				XPAR_AXI_DMA_0_DEVICE_ID
#define AXI_DMA_BASE_ADDR  			XPAR_AXI_DMA_0_BASEADDR
#define AXI_DMA_COALESCE			10
#define AXI_DMA_NUMBER_OF_TRANSFERS 100

#define MEM_BASE_ADDR			(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000)

#define AXI_DMA_RX_BD_SPACE_BASE	(MEM_BASE_ADDR)
#define AXI_DMA_RX_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0000FFFF)
#define AXI_DMA_RX_BD_SPACE 		AXI_DMA_RX_BD_SPACE_HIGH - AXI_DMA_RX_BD_SPACE_BASE + 1
#define AXI_DMA_RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x000300000)
#define AXI_DMA_RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x0004FFFFF)

#define AXI_DMA_RX_INTR_ID			XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR

extern u32 axiDmaIntCount;
extern u32 axiDmaTransferCount;
extern u32 Error;

typedef void (*AXI_DMA_ProcessBufferDelegate) (u32 sendBufferAddr, int buffSizeBytes, int dataLen);

int AXI_DMA_Init(int buffLen, AXI_DMA_ProcessBufferDelegate processBuffer);
void AXI_DMA_Reset();
int AXI_DMA_SetupRx(u32 cnt, u32 len);

#endif /* SRC_AXITAR_AXIDMA_H_ */
