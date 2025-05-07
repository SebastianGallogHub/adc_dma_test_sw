/*
 * xuart0_xdmaps.h
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

#ifndef SRC_UART_UART_DMAPS_H_
#define SRC_UART_UART_DMAPS_H_

#include "xil_types.h"
#include "xdmaps.h"

#define DMA_DEVICE_ID 		XPAR_XDMAPS_1_DEVICE_ID
#define DMA_DONE_INTR_0		XPAR_XDMAPS_0_DONE_INTR_0
#define DMA_FAULT_INTR		XPAR_XDMAPS_0_FAULT_INTR
#define DMA_CHANNEL			0

void DMAPS_Init();
int DMAPS_Done();
void DMAPS_ConfigSend(u32 src, u32 dst, unsigned int transferLen);
void DMAPS_Send();

#endif /* SRC_UART_UART_DMAPS_H_ */
