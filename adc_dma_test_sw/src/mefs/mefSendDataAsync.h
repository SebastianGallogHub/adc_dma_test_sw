/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : mefSendDataAsync.h
 * Descripción         : Definiciones de constantes y funciones para implementación
 * 						 de mef de envío contínuo y asincrónico de datos por UART.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 18/04/2025
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

#ifndef SRC_MEFS_MEFSENDDATAASYNC_H_
#define SRC_MEFS_MEFSENDDATAASYNC_H_

void mefSendDataAsync_Init();
void mefSendDataAsync_Reset();
void mefSendDataAsync_Cancel();
int mefSendDataAsync();

#endif /* SRC_MEFS_MEFSENDDATAASYNC_H_ */
