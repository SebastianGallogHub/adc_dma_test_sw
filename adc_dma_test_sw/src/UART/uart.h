/*
 * uart.h
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

#ifndef SRC_UART_UART_H_
#define SRC_UART_UART_H_

#include "xparameters.h"
#include "xil_types.h"

#define UART_DEVICE_ID			XPAR_XUARTPS_0_DEVICE_ID
#define UART_INT_IRQ_ID			XPAR_XUARTPS_0_INTR
#define UART_TX_RX_FIFO_ADDR	XPAR_XUARTPS_0_BASEADDR + XUARTPS_FIFO_OFFSET
#define UART_TX_FIFO_DEPTH		64

#define UART_MIN_DELAY_PER_BYTE_SEND 300

#define MEM_BASE_ADDR			(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000)

// Copia de buffer para envío
#define UART_TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x000500000)
#define UART_TX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x0006FFFFF)
#define UART_TX_BUFFER_SPACE	UART_TX_BUFFER_HIGH - UART_TX_BUFFER_BASE + 1

#define BUFFER_SIZE	40

void UART_Init();
void UART_SetupRx();

void UART_SendAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen);
void UART_SendBufferAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen, int cancel);
void UART_SendBufferAsync_Cancel();

int UART_DoneSendBuffer();
int UART_DoneTx();

#endif /* SRC_UART_UART_H_ */
