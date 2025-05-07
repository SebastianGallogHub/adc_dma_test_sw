/*
 * sd_card.h
 *
 *  Created on: Apr 15, 2025
 *      Author: sebas
 */

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
