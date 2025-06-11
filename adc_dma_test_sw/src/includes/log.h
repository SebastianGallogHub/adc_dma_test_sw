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

#ifndef SRC_INCLUDES_LOG_H_
#define SRC_INCLUDES_LOG_H_

#include "xstatus.h"
#include "xil_printf.h"

//#define LOG_OFF
#ifndef LOG_OFF
#define LOG(level, fmt, ...) do { \
	if (level){volatile int lvl = level;\
	while(lvl){lvl--; xil_printf("\t");}}\
    xil_printf ("" fmt, ##__VA_ARGS__); \
    xil_printf ("\r\n"); \
} while (0)

#define LOG_LINE LOG(2, ">--------------------------------<");

#define LOG_CLEAR_SCREEN xil_printf("%c%c%c%c%c[H", 0x1B, 0x5B, 0x32, 0x4A, 0x1B)
#else
#define LOG(level, fmt, ...) do{}while(0)

#define LOG_LINE LOG(2, ">--------------------------------<");

#define LOG_CLEAR_SCREEN xil_printf("%c%c%c%c%c[H", 0x1B, 0x5B, 0x32, 0x4A, 0x1B)
#endif


#define B2KB(bytes) bytes/1024
#define B2MB(bytes) KB(bytes)/1024
#define B2GB(bytes) MB(bytes)/1024

#endif /* SRC_ASSERT_H_ */
