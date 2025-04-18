/***************************** Include Files *******************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "mefSerialToBin.h"
#include "serial_port.h"

/************************** Constant Definitions **************************/
#define to_us(s) s * 1000 /*ms*/ * 1000 /*us*/

#define ZMODADC1410_RESOLUTION 3.21f /*mv*/
#define to_cad(mv) (uint32_t)(mv / ZMODADC1410_RESOLUTION)

#define requires_param(c) c == CMD_CH0_H || c == CMD_CH1_H

/**************************** Type Definitions ******************************/
typedef enum
{
    CMD_HEADER = 0x25,
    CMD_START = 0x01,
    CMD_STOP = 0x02,
    CMD_CH0_H = 0xA0,
    CMD_CH1_H = 0xB0,
} SERIAL_COMMAND;

/************************** Function Prototypes *****************************/
void *rx_thread_func(void *arg);
void serial_SendCommand(SERIAL_COMMAND c, ...);

/************************** Variable Definitions ***************************/

/****************************************************************************/

int main()
{
    char resp = 0;
    int tEnsayo_s = 120;
    int tPausa_s = 3;
    uint16_t hist0_low = 1000, hist0_high = 3000;
    uint16_t hist1_low = 1000, hist1_high = 3000;

    if (serial_Init("/dev/ttyUSB1"))
        return 1;

    pthread_t rx_thread;
    if (pthread_create(&rx_thread, NULL, rx_thread_func, NULL) != 0)
    {
        perror("No se pudo crear el hilo de recepción");
        serial_Close();
        return 1;
    }

    do
    {
        scanf(" %c", &resp);
    } while (resp != 's');

    while (resp == 's')
    {
        serial_SendCommand(CMD_CH0_H, to_cad(hist0_high) << 16 | to_cad(hist0_low));
        usleep(1000);

        serial_SendCommand(CMD_CH1_H, to_cad(hist1_high) << 16 | to_cad(hist1_low));
        usleep(1000);

        usleep(1000000);

        mefSerialToBin_StartReceivingData();

        serial_SendCommand(CMD_START);

        usleep(to_us(tEnsayo_s)); // Se detiene el hilo para recibir datos

        serial_SendCommand(CMD_STOP);

        usleep(to_us(tPausa_s)); // Pausa para terminar de recibir todo lo que falte

        mefSerialToBin_StopReceivingData();

        scanf(" %c", &resp);
    }

    pthread_join(rx_thread, NULL);

    return 0;
}

void serial_SendCommand(SERIAL_COMMAND c, ...)
{
    serial_WriteByte((char)CMD_HEADER);
    serial_WriteByte((char)c);

    va_list args;
    va_start(args, c);

    if (requires_param(c))
    {
        uint32_t param = va_arg(args, uint32_t);

        // Enviar el parámetro como 4 bytes (big endian o little endian según necesites)
        serial_WriteByte((param >> 24) & 0xFF);
        serial_WriteByte((param >> 16) & 0xFF);
        serial_WriteByte((param >> 8) & 0xFF);
        serial_WriteByte((param >> 0) & 0xFF);
    }

    va_end(args);
}

void *rx_thread_func(void *arg)
{
    char buffer;
    int bytes_read;

    while (1)
    {
        bytes_read = serial_ReadByte(&buffer);

        mefSerialToBin(buffer, bytes_read);
    }

    return NULL;
}

/**************************************************************************************************/
