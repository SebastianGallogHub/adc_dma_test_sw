/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : sd_card.c
 * Descripción         : Archivo de implementación de las herramientas
 * 						 de lectura/escritra bajo nivel sobre memoria SD.
 * 						 Se gestiona un ringbuffer sobre esta memoria.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 15/04/2025
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

/***************************** Include Files *******************************/
#include "../SD_CARD/sd_card.h"

#include "xil_printf.h"
#include "diskio.h"
#include "sleep.h"

#include "../includes/log.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
DWORD total_sectors;
DWORD limit;

u32 sector_wr_idx = 0;
u32 sector_rd_idx = 0;

/****************************************************************************/

int SD_Init(){
	DSTATUS res;

	res = disk_initialize(0);
	if (res != RES_OK) {
		xil_printf("Fallo en disk_initialize\r\n");
		return 1;
	}

	res = disk_ioctl(0, GET_SECTOR_COUNT, &total_sectors);
	if (res != RES_OK) {
		xil_printf("Fallo en GET_SECTOR_COUNT\r\n");
		return 1;
	}

	return 0;
}

void SD_ResetRB(){
	sector_wr_idx = 0;
	sector_rd_idx = 0;
	limit = 0;
}


int SD_WriteSectors(BYTE *buffer, UINT countSectors){
	DSTATUS res;

	res = disk_write(0, buffer, sector_wr_idx, countSectors);

	if (res == RES_OK){
		sector_wr_idx += countSectors;
		limit = total_sectors - countSectors;
		if(sector_wr_idx > limit)
				sector_wr_idx = 0;
		return 1;
	} else{
		return -1;
	}
}

int SD_GetSectorsToRead(){
	if(sector_rd_idx <= sector_wr_idx)
		return sector_wr_idx - sector_rd_idx;
	else
		return limit - sector_rd_idx;
}

int SD_ReadNextSector(BYTE *out_buffer){
	if(sector_rd_idx >= sector_wr_idx) return 0; //Ya no queda nada por leer

	DSTATUS res;

	res = disk_read(0, out_buffer, sector_rd_idx, 1);
//	usleep(100000); // si
//	usleep(10000); 	// si
//	usleep(1000);	// no
//	usleep(5000);	// si
//	usleep(3000);	// si
//	usleep(2000);	// no
	usleep(2500);	// si
//	usleep(2300);	// no
//	usleep(2400);	// no
//	usleep(2450);	// no

	if (res == RES_OK) {
		sector_rd_idx ++;
		if(sector_rd_idx > limit)
			sector_rd_idx = 0;

		return 1;
	}else {
		return -1;
	}
}


