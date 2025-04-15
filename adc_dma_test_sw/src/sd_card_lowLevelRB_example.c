/*
 * sd_card_lowLevelRB_example.c
 *
 *  Created on: Apr 14, 2025
 *      Author: sebas
 */
#define MAIN_SD_CARD_LL_RB
#ifdef MAIN_SD_CARD_LL_RB
/***************************** Include Files *******************************/
#include "xil_printf.h"
#include "xparameters.h"
#include "diskio.h"
#include "sleep.h"

#include "InterruptSystem/interruptSystem.h"
#include "UART_DMA/xuartps_0.h"

/************************** Constant Definitions **************************/
#define SECTOR_SIZE 		512
#define WORDS_PER_SECTOR 	(SECTOR_SIZE / sizeof(u32))

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
u32 sector_wr_idx = 0;
u32 sector_wr_buffer[WORDS_PER_SECTOR] __attribute__ ((aligned (32)));

u32 sector_rd_idx = 0;
u32 sector_rd_buffer[WORDS_PER_SECTOR] __attribute__ ((aligned (32)));

/****************************************************************************/

int main() {
	u32 i;

    DWORD total_sectors;
    DSTATUS res;

    UARTPS_0_Init();

    SetupIntrSystem();

    UARTPS_0_StartRx();

    xil_printf("Inicializando SD RAW como buffer circular...\r\n");

    res = disk_initialize(0);
    if (res != RES_OK) {
    	xil_printf("Fallo en disk_initialize\r\n");
        return -1;
    }

    res = disk_ioctl(0, GET_SECTOR_COUNT, &total_sectors);
    if (res != RES_OK) {
    	xil_printf("Fallo en GET_SECTOR_COUNT\r\n");
        return -1;
    }

    xil_printf("Total sectores disponibles: %u\r\n", total_sectors);

    u32 val = 65532 << 16 | 120;
    u32 sectors_to_send = 10;

    for (i = 0; i < WORDS_PER_SECTOR; i++) {
		sector_wr_buffer[i] = val;
	}

    for (u32 sector = 0; sector < sectors_to_send; sector++) {
        res = disk_write(0, (BYTE *)sector_wr_buffer, sector, 1);
        if (res != RES_OK) {
            xil_printf("Error al escribir sector %u\r\n", sector);
            return -1;
        }
    }

    xil_printf("Escritura SD completada\r\n");

    xil_printf("Enviando datos...\r\n");
    xil_printf("&");
    usleep(100);

    u32 sector = 0;
    while(sector < sectors_to_send){
        //Espero a que envíe el buffer entero
    	if (UARTPS_0_DoneSendBuffer()){

    		// Leo el sector actual de la SD a DDR
    		res = disk_read(0, (BYTE *)sector_rd_buffer, sector, 1);
			if (res != RES_OK) {
				xil_printf("Error al leer sector %u\r\n", sector);
				return -1;
			}

			// Envío el sector actual desde DDR
			UARTPS_0_SendBufferAsync((u32)sector_rd_buffer, SECTOR_SIZE, sizeof(u32));

			// Avanzo al siguiente sector
			sector ++;
        }
    }

    while(1){
		if(UARTPS_0_DoneSendBuffer()){
			xil_printf("&");
			break;
		}
	}

    xil_printf("Lectura y transmisión diferidas completadas\r\n");
    return 0;
}

#endif


