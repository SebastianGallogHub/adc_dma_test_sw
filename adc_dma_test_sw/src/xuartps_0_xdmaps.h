/*
 * xuart0_xdmaps.h
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

#ifndef SRC_XUARTPS_0_XDMAPS_H_
#define SRC_XUARTPS_0_XDMAPS_H_

#include "xil_types.h"
#include "xdmaps.h"

#define DMA_DEVICE_ID 		XPAR_XDMAPS_1_DEVICE_ID
#define DMA_DONE_INTR_0		XPAR_XDMAPS_0_DONE_INTR_0
#define DMA_FAULT_INTR		XPAR_XDMAPS_0_FAULT_INTR
#define DMA_CHANNEL			0
#define DMA_LENGTH			1024	/* Length of the Dma Transfers */
#define TIMEOUT_LIMIT 		0x2000	/* Loop count for timeout */

static XDmaPs_Cmd DmaCmd;

void DMAPS_Init();

int DMAPS_Done();

void DMAPS_ConfigSend(u32 src, u32 dst,
		int burstSize, int burstLen, unsigned int transferLen);

void DMAPS_Send();

#endif /* SRC_XUARTPS_0_XDMAPS_H_ */
