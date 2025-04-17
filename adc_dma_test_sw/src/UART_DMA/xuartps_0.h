/*
 * xuartps_0.h
 *
 *  Created on: Mar 24, 2025
 *      Author: sebas
 */

#ifndef SRC_UART_DMA_XUARTPS_0_H_
#define SRC_UART_DMA_XUARTPS_0_H_

#include "xparameters.h"

#define UART_DEVICE_ID			XPAR_XUARTPS_0_DEVICE_ID
#define UART_INT_IRQ_ID			XPAR_XUARTPS_0_INTR
#define UART_TX_RX_FIFO_ADDR	XPAR_XUARTPS_0_BASEADDR + XUARTPS_FIFO_OFFSET
#define UART_TX_FIFO_DEPTH		64

#define UART_MIN_DELAY_PER_BYTE_SEND 300

#define MEM_BASE_ADDR			(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000)

// Copia de buffer para envío
#define UART_0_TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x000500000)
#define UART_0_TX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x0006FFFFF)
#define UART_0_TX_BUFFER_SPACE		UART_0_TX_BUFFER_HIGH - UART_0_TX_BUFFER_BASE + 1

#define BUFFER_SIZE	40

#define COMMAND_FORMAT_HEADER '%'

typedef enum{
	CMD_START = 0x01,
	CMD_STOP  = 0x02,
	CMD_CHA_H_L = 0xA0,
	CMD_CHA_H_H = 0xA1,
	CMD_CHB_H_L = 0xB0,
	CMD_CHB_H_H = 0xB1,
}UART_COMMAND;

void UARTPS_0_Init();

void UARTPS_0_StartRx();

void UARTPS_0_SendAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen);

void UARTPS_0_SendBufferAsync(u32 sendBufferAddr, int buffSizeBytes, int dataLen);

UART_COMMAND UART_0_GetCommand();

u8 UART_0_HasParameter();

u16 UART_0_GetParameter();

int UARTPS_0_DoneSendBuffer();

int UARTPS_0_DoneTx();



//void UARTPS_0_Test();

#endif /* SRC_UART_DMA_XUARTPS_0_H_ */
