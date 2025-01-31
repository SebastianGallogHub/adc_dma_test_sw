/*
 * assert.h
 *
 *  Created on: Sep 5, 2024
 *      Author: sebas
 */

#ifndef SRC_ASSERT_H_
#define SRC_ASSERT_H_

#include "xstatus.h"

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

#endif /* SRC_ASSERT_H_ */
