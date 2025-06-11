/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : uart_dmaps.h
 * Descripción         : Definiciones de constantes y funciones para
 * 						 transacciones memoria a periférico mediante canal
 * 						 DMAPS.
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
