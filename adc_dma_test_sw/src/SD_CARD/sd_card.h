/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : sd_card.h
 * Descripción         : Definiciones de constantes y funciones para lectura/escritura
 * 						 a bajo nivel en memoria SD.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 15/04/2024
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

#ifndef SRC_SD_CARD_SD_CARD_H_
#define SRC_SD_CARD_SD_CARD_H_

#include "diskio.h"

#define SD_SECTOR_SIZE 					512
#define SD_WORDS_PER_SECTOR(dataSize) 	SD_SECTOR_SIZE / dataSize

int SD_Init();
int SD_GetSectorsToRead();
int SD_SetSectorsToWrite(unsigned int);
int SD_WriteSectors(unsigned char *buffer, unsigned int countSectors);
int SD_ReadNextSector(unsigned char *out_buffer);
void SD_ResetRB();


#endif /* SRC_SD_CARD_SD_CARD_H_ */
