/*
 * uart_config.h
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#ifndef SRC_UART_CONFIG_H_
#define SRC_UART_CONFIG_H_

#include "xparameters.h"

#define UART_DEVICE_ID        XPAR_XUARTPS_0_DEVICE_ID

#define UART_INT_IRQ_ID       XPAR_XUARTPS_0_INTR

#define RECV_BUFFER_SIZE    3

int UART_Init();

#endif /* SRC_UART_CONFIG_H_ */
