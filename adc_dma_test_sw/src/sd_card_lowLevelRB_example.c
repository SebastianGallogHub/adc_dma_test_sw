/*
 * sd_card_lowLevelRB_example.c
 *
 *  Created on: Apr 14, 2025
 *      Author: sebas
 */
//#define MAIN_SD_CARD_LL_RB
#ifdef MAIN_SD_CARD_LL_RB
/***************************** Include Files *******************************/
#include "xil_printf.h"
#include "xparameters.h"
#include "xuartps.h"
#include "diskio.h"

/************************** Constant Definitions **************************/
#define UART_DEVICE_ID XPAR_XUARTPS_0_DEVICE_ID
#define SECTOR_SIZE 512
#define WORDS_PER_SECTOR (SECTOR_SIZE / sizeof(u32))

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
XUartPs Uart_PS;
u32 write_buffer[WORDS_PER_SECTOR];
u32 read_buffer[WORDS_PER_SECTOR];

/****************************************************************************/

int init_uart(XUartPs *UartInst) {
    XUartPs_Config *Config = XUartPs_LookupConfig(UART_DEVICE_ID);
    if (!Config) return XST_FAILURE;

    int Status = XUartPs_CfgInitialize(UartInst, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) return Status;

    XUartPs_SetBaudRate(UartInst, 115200);
    return XST_SUCCESS;
}

void send_uart_str(const char *msg) {
    XUartPs_Send(&Uart_PS, (u8 *)msg, strlen(msg));
}

int main() {
	u32 i;
    DWORD total_sectors;
    DSTATUS res;

    init_uart(&Uart_PS);
    send_uart_str("Inicializando SD RAW como buffer circular...\r\n");

    res = disk_initialize(0);
    if (res != RES_OK) {
        send_uart_str("Fallo en disk_initialize\r\n");
        return -1;
    }

    res = disk_ioctl(0, GET_SECTOR_COUNT, &total_sectors);
    if (res != RES_OK) {
        send_uart_str("Fallo en GET_SECTOR_COUNT\r\n");
        return -1;
    }

    xil_printf("Total sectores disponibles: %lu\r\n", total_sectors);
return 0;
    u32 val = 10;
    for (u32 sector = 0; sector < total_sectors; sector++) {
        for (i = 0; i < WORDS_PER_SECTOR; i++) {
            write_buffer[i] = val++;
        }

        res = disk_write(0, (BYTE *)write_buffer, sector, 1);
        if (res != RES_OK) {
            xil_printf("Error al escribir sector %lu\r\n", sector);
            return -1;
        }
    }

    send_uart_str("Escritura circular completada\r\n");

    val = 1000;
    for (u32 sector = 0; sector < total_sectors; sector++) {
        res = disk_read(0, (BYTE *)read_buffer, sector, 1);
        if (res != RES_OK) {
            xil_printf("Error al leer sector %lu\r\n", sector);
            return -1;
        }

        for (i = 0; i < WORDS_PER_SECTOR; i++) {
            xil_printf("Dato %lu: %lu\r\n", val++, read_buffer[i]);
        }
    }

    send_uart_str("Lectura y transmisión circular completadas\r\n");
    return 0;
}



#endif


