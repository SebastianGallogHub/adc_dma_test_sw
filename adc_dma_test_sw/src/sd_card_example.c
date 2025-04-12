/*
 * sd_card_example.c
 *
 *  Created on: Apr 12, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "xparameters.h"
#include "xil_printf.h"
#include "ff.h"
/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

FATFS fatfs;
FIL fil;
FRESULT rc;

/****************************************************************************/

#define MAIN_SD_CARD
#ifdef MAIN_SD_CARD

int main() {
    const char *filename = "test.txt";
    UINT bw;
    char data[] = "Hola SD card desde bare-metal!\n";

    xil_printf("Montando SD...\n");
    rc = f_mount(&fatfs, "0:/", 0);
    if (rc != FR_OK) {
        xil_printf("Error al montar la SD: %d\n", rc);
        return -1;
    }

    xil_printf("Abriendo archivo...\n");
    rc = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (rc != FR_OK) {
        xil_printf("Error al abrir archivo: %d\n", rc);
        return -1;
    }

    xil_printf("Escribiendo datos...\n");
    rc = f_write(&fil, data, sizeof(data)-1, &bw);
    if (rc != FR_OK || bw != (sizeof(data)-1)) {
        xil_printf("Error al escribir archivo\n");
    } else {
        xil_printf("Escritura OK! Bytes escritos: %d\n", bw);
    }

    f_close(&fil);
    xil_printf("Hecho.\n");
    return 0;
}

#endif
