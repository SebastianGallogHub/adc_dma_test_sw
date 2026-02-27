/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : usb.h
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

#ifndef SRC_USB_USB_H_
#define SRC_USB_USB_H_

#include "xil_types.h"

//#define USB_COMM

#define USB_NUM_BUFS	 		16
#define USB_MAX_PACKET_SIZE 	512

int USB_Init();

void USB_SendBuffer(void *sendBufferAddr, int buffSizeBytes);
int USB_DoneSendBuffer();


#endif /* SRC_USB_USB_H_ */
