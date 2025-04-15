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

#include "SD_CARD/sd_card.h"
#include "UART_DMA/xuartps_0.h"
#include "InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/
#define WORDS_PER_SECTOR SD_WORDS_PER_SECTOR(sizeof(u32))

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
u32 sector_wr_buffer[WORDS_PER_SECTOR] __attribute__ ((aligned (32)));
u32 sector_rd_buffer[WORDS_PER_SECTOR] __attribute__ ((aligned (32)));

/****************************************************************************/

int main() {

    UARTPS_0_Init();
	SD_Init();

    SetupIntrSystem();

    UARTPS_0_StartRx();

    xil_printf("Inicializando SD RAW como buffer circular...\r\n");

    u32 val = 65532 << 16 | 120;
    u32 sectors_to_send = 100;
    int res;

    for (u32 i = 0; i < WORDS_PER_SECTOR; i++) {
		sector_wr_buffer[i] = val++;
	}

    xil_printf("Escribiendo y enviando datos...\r\n");
    xil_printf("&");

    u32 sended = 0;
    u32 sector = 0;

    res = SD_WriteNextSector((unsigned char *) sector_wr_buffer);
	if(res < 0){
		xil_printf("Error al escribir sector %u\r\n", sector);
		return -1;
	}
	sector ++;

    do{
    	// Escribo un sector a la vez que leo mientras pueda
    	if (sector < sectors_to_send){
			res = SD_WriteNextSector((unsigned char *) sector_wr_buffer);
			if(res < 0){
				xil_printf("Error al escribir sector %u\r\n", sector);
				return -1;
			}
			sector ++;
    	}

    	//Espero a que envíe el buffer entero porque sino sobreescribo la ddr
    	if (UARTPS_0_DoneSendBuffer()){

    		// Leo el siguiente sector de la SD a DDR
    		res = SD_ReadNextSector((unsigned char *)sector_rd_buffer);
			if(res < 0){
				xil_printf("Error al leer sector\r\n");
				return -1;
			}

			// Envío el sector actual desde DDR
			if (res == 1){
				UARTPS_0_SendBufferAsync((u32)sector_rd_buffer, SD_SECTOR_SIZE, sizeof(u32));
				sended++;
			}
        }
    }while(SD_SectorsToRead() > 0);

    while(1){
		if(UARTPS_0_DoneSendBuffer()){
			xil_printf("&");
			break;
		}
	}

    xil_printf("Lectura y transmisión diferidas completadas, %u sectores\r\n", sended);
    return 0;
}

#endif


