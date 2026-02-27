/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : log.h
 * Descripción         : Definiciones de constantes y macros para realizar
 * 						 logs por UART durante debug.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 05/09/2024
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

#ifndef SRC_INCLUDES_COMMANDS_H_
#define SRC_INCLUDES_COMMANDS_H_

#define CMD_HEADER 0x25

typedef enum{
	CMD_NONE = 0,
	CMD_START = 0x01,
	CMD_STOP  = 0x02,
	CMD_CH0_H = 0xA0,
	CMD_CH1_H = 0xB0,
	CMD_GET_CONF = 0xF0,
}TAR_COMMAND;

#endif /* SRC_INCLUDES_COMMANDS_H_ */
