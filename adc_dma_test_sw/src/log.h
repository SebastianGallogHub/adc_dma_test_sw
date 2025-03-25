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
