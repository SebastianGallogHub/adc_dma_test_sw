/*
 * xuartps_0.h
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

#ifndef SRC_XUARTPS_0_H_
#define SRC_XUARTPS_0_H_

#include "xparameters.h"

#define UART_DEVICE_ID		XPAR_XUARTPS_0_DEVICE_ID
#define UART_INT_IRQ_ID		XPAR_XUARTPS_0_INTR
#define UART_TX_RX_FIFO_ADDR	XPAR_XUARTPS_0_BASEADDR + XUARTPS_FIFO_OFFSET

#define TEST_BUFFER_SIZE	40

void UARTPS_0_Init();

int UARTPS_0_SendAsync();

void UARTPS_0_Test();

#endif /* SRC_XUARTPS_0_H_ */
