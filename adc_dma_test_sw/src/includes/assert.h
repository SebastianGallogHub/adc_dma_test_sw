/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : assert.h
 * Descripción         : Definiciones de constantes y funciones para verificación
 * 						 de puntos de control para debug y control de flujo del programa
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

#ifndef SRC_INCLUDES_ASSERT_H_
#define SRC_INCLUDES_ASSERT_H_

#include "xstatus.h"
#include "xil_printf.h"

/*
 * Verifica que condición = true
 * De lo contrario sale de la función con XST_FAILURE
 * Puede especificarse un formato y argumentos válidos
 * para xil_printf como salida de error
 * */
#define ASSERT(condition, fmt, ...) do { \
    if (!(condition)) { \
    	xil_printf ("" fmt "\r\n", ##__VA_ARGS__); \
    	return XST_FAILURE; \
    } \
} while (0)

#define ASSERT_void(condition, fmt, ...) do { \
    if (!(condition)) { \
    	xil_printf ("" fmt "\r\n", ##__VA_ARGS__); \
    	return; \
    } \
} while (0)


/*
 * Verifica que status = XST_SUCCESS
 * De lo contrario sale de la función con XST_FAILURE
 * Puede especificarse un texto plano como salida de error
 * */
static long st;
#define ASSERT_SUCCESS(status, fmt, ...) do { \
	st = status;	\
    if ((st) != XST_SUCCESS) { \
    	xil_printf ("(status=%d)\t", st); \
    	xil_printf ("" fmt "\r\n", ##__VA_ARGS__); \
    	return XST_FAILURE; \
    } \
} while (0)
#define ASSERT_SUCCESS_void(status, fmt, ...) do { \
	st = status;	\
    if ((st) != XST_SUCCESS) { \
    	xil_printf ("(status=%d)\t", st); \
    	xil_printf ("" fmt "\r\n", ##__VA_ARGS__); \
    	return;\
    } \
} while (0)

#endif /* SRC_INCLUDES_ASSERT_H_ */
