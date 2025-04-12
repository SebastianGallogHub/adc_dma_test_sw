/*
 * sd_card_lowLevel_example.c
 *
 *  Created on: Apr 12, 2025
 *      Author: sebas
 */
#define MAIN_SD_CARD_LL
#ifdef MAIN_SD_CARD_LL
/***************************** Include Files *******************************/

#include "diskio.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "xil_types.h"

/************************** Constant Definitions **************************/
#define SECTOR_BASE 2048   // Cambiar a 0 si no tenés tabla de particiones
#define SECTOR_SIZE 512   // bytes

#define DATA_SIZE   sizeof(u32)

#define BUFFER_SIZE SECTOR_SIZE/DATA_SIZE     // 128 enteros de 4 bytes = 512 bytes

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
u32 tx_buffer[BUFFER_SIZE];
u32 rx_buffer[BUFFER_SIZE];

/****************************************************************************/

void uart_send_uint32(u32 value) {
    xil_printf("%u\n", value);  // O usá la función que tengas en tu sistema
}

void write_data_to_sd() {
    u32 current_value = 1000;
    u32 sector = SECTOR_BASE;

    while (current_value <= 2000) {
        u32 count = 0;
        for (; count < BUFFER_SIZE && current_value <= 2000; count++) {
            tx_buffer[count] = current_value++;
        }
        // Rellenar el resto si no está lleno
        for (; count < BUFFER_SIZE; count++) {
            tx_buffer[count] = 0;
        }

        DSTATUS stat = disk_write(0, (BYTE *)tx_buffer, sector, 1);
        if (stat != RES_OK) {
            xil_printf("Error al escribir en el sector %lu\r\n", sector);
            return;
        }

        sector++;
    }

    xil_printf("Escritura completa.\r\n");
}

void read_data_from_sd_and_uart() {
    u32 total_values = 1001;
    u32 sector = SECTOR_BASE;
    u32 values_read = 0;

    while (values_read < total_values) {
        if (disk_read(0, (BYTE *)rx_buffer, sector, 1) != RES_OK) {
            xil_printf("Error al leer sector %lu\r\n", sector);
            return;
        }

        for (u32 i = 0; i < BUFFER_SIZE && values_read < total_values; i++) {
            uart_send_uint32(rx_buffer[i]);
            values_read++;
        }

        sector++;
    }

    xil_printf("Lectura y transmisión completa.\r\n");
}

int main() {
	if (disk_initialize(0) != RES_OK) {
	    xil_printf("Error al inicializar la SD.\r\n");
	    return -1;
	}
    write_data_to_sd();
    read_data_from_sd_and_uart();
    return 0;
}

#endif
