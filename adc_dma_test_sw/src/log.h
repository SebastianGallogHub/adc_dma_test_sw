/*
 * assert.h
 *
 *  Created on: Sep 5, 2024
 *      Author: sebas
 */

#ifndef SRC_LOG_H_
#define SRC_LOG_H_

#include "xstatus.h"
#include "xil_printf.h"

/*
 * Verifica que condición = true
 * De lo contrario sale de la función con XST_FAILURE
 * Puede especificarse un formato y argumentos válidos
 * para xil_printf como salida de error
 * */
#define LOG(level, fmt, ...) do { \
	if (level){volatile int lvl = level;\
	while(lvl){lvl--; xil_printf("\t");}}\
    xil_printf ("" fmt, ##__VA_ARGS__); \
    xil_printf ("\r\n"); \
} while (0)

#define LOG_LINE LOG(2, ">--------------------------------<");

#define CLEAR_SCREEN xil_printf("%c%c%c%c%c[H", 0x1B, 0x5B, 0x32, 0x4A, 0x1B)

#define B2KB(bytes) bytes/1024
#define B2MB(bytes) KB(bytes)/1024
#define B2GB(bytes) MB(bytes)/1024

#endif /* SRC_ASSERT_H_ */
