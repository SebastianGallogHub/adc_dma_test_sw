/*
 * xuartps_0.h
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

#ifndef SRC_XUARTPS_0_H_
#define SRC_XUARTPS_0_H_

#include "xparameters.h"

#define UART_DEVICE_ID			XPAR_XUARTPS_0_DEVICE_ID
#define UART_INT_IRQ_ID			XPAR_XUARTPS_0_INTR
#define UART_TX_RX_FIFO_ADDR	XPAR_XUARTPS_0_BASEADDR + XUARTPS_FIFO_OFFSET
#define UART_TX_FIFO_DEPTH		64

#define UART_MIN_DELAY_PER_BYTE_SEND 300

#define BUFFER_SIZE	40

void UARTPS_0_Init();
void UARTPS_0_StartRx();
void UARTPS_0_SendAsync(u32 sendBufferAddr, int buffSizeBytes);
int UARTPS_0_DoneTx();

void UARTPS_0_Test();

#endif /* SRC_XUARTPS_0_H_ */
