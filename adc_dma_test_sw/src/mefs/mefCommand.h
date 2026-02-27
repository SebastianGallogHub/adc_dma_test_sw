/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : mefCommand.h
 * Descripción         : Definiciones de constantes y funciones para implementar
 * 						 la recepción de comandos por UART.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 18/04/2024
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

#ifndef SRC_UART_UART_MEFCOMMAND_H_
#define SRC_UART_UART_MEFCOMMAND_H_

#include "xil_types.h"
#include "../includes/commands.h"

void mefCommand(u8 chr);

TAR_COMMAND mefCommand_GetCommand();
u8 mefCommand_HasParameter();
u32 mefCommand_GetParameter();

#endif /* SRC_UART_UART_MEFCOMMAND_H_ */
