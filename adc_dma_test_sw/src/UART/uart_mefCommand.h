/*
 * uart_mefCommand.h
 *
 *  Created on: Apr 18, 2025
 *      Author: sebas
 */

#ifndef SRC_UART_UART_MEFCOMMAND_H_
#define SRC_UART_UART_MEFCOMMAND_H_

#include "xil_types.h"

#define CMD_HEADER 0x25

typedef enum{
	CMD_NONE = 0,
	CMD_START = 0x01,
	CMD_STOP  = 0x02,
	CMD_CH0_H = 0xA0,
	CMD_CH1_H = 0xB0,
	CMD_GET_CONF = 0xF0,
}UART_COMMAND;

void UART_mefCommand(u8 chr);

UART_COMMAND UART_GetCommand();
u8 UART_HasParameter();
u32 UART_GetParameter();

#endif /* SRC_UART_UART_MEFCOMMAND_H_ */
