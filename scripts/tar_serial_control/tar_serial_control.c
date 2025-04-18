/***************************** Include Files *******************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "mefSerialToBin.h"
#include "serial_port.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/
typedef enum
{
    CMD_HEADER = 0x25,
    CMD_START = 0x01,
    CMD_STOP = 0x02,
    CMD_CH0_H_L = 0xA0,
    CMD_CH0_H_H = 0xA1,
    CMD_CH1_H_L = 0xB0,
    CMD_CH1_H_H = 0xB1,
} SERIAL_COMMAND;

/************************** Function Prototypes *****************************/
void *rx_thread_func(void *arg);

/************************** Variable Definitions ***************************/

/****************************************************************************/

int main()
{
    char resp = 0;
    int tEnsayo_s = 120;
    int tPausa_s = 3;

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
        mefSerialToBin_StartReceivingData();

        serial_WriteByte((char)CMD_HEADER);
        serial_WriteByte((char)CMD_START);

        usleep(tEnsayo_s * 1000 /*ms*/ * 1000 /*us*/); // Se detiene el hilo para recibir datos

        serial_WriteByte((char)CMD_HEADER);
        serial_WriteByte((char)CMD_STOP);

        usleep(tPausa_s * 1000 /*ms*/ * 1000 /*us*/); // Pausa para terminar de recibir todo lo que falte

        mefSerialToBin_StopReceivingData();

        scanf(" %c", &resp);
    }

    pthread_join(rx_thread, NULL);

    return 0;
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
