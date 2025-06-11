/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : uart.h
 * Descripción         : Definiciones de constantes y funciones para la
 * 						 comunicación full-duplex mediante UART.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 24/03/2024
 * Fecha de modificación: 11/06/2025
 * Versión             : v1.0
 *
 * Institución         : Universidad Nacional de Rosario (UNR)
 * Carrera             : Ingeniería Electrónica
 *
 * Derechos reservados:
 * Este código ha sido desarrollado en el marco del Proyecto Final de Ingeniería
 * por Sebastián Nahuel Gallo. Su uso está autorizado únicamente por la
 * Comisión Nacional de Energía Atómica (CNEA) con fines internos.
 * Queda prohibida su reproducción, modificación o distribución sin
 * autorización expresa por escrito del autor.
 ***************************************************************/

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
