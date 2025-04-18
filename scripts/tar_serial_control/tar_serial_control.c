/***************************** Include Files *******************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "mefSerialToBin.h"
#include "serial_port.h"

/************************** Constant Definitions **************************/
#define ZMODADC1410_RESOLUTION 3.21f /*mv*/
#define to_cad(mv) (uint16_t)(mv / ZMODADC1410_RESOLUTION)
#define to_hist(low, high) ((uint32_t)high) << 16 | low
#define to_us(s) s * 1000 /*ms*/ * 1000 /*us*/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
void *rx_thread_func(void *arg);

/************************** Variable Definitions ***************************/

/****************************************************************************/

int main()
{
    char resp = 0;
    int tEnsayo_s = 1;
    int tPausaInicio_s = 1;
    int tPausaFin_s = 3;
    uint16_t hist0_low = 1000, hist0_high = 3000;
    uint16_t hist1_low = 980, hist1_high = 3080;

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
        // Configuro la histéresis de ambos canales
        serial_SendCommand(CMD_CH0_H, to_hist(to_cad(hist0_low), to_cad(hist0_high)));

        serial_SendCommand(CMD_CH1_H, to_hist(to_cad(hist1_low), to_cad(hist1_high)));

        usleep(to_us(tPausaInicio_s)); // Pausa para esperar la respuesta de log del TAR

        printf("Iniciando ensayo %d s\n", tEnsayo_s);

        // Aviso a la mef que captura el archivo crudo que se prepare para recibir datos
        mefSerialToBin_StartReceivingData();

        usleep(to_us(1));

        // Envío la señal de inicio a TAR
        serial_SendCommand(CMD_START);

        usleep(to_us(tEnsayo_s)); // Se detiene el hilo para recibir datos

        // Envío la señal de fin a TAR
        serial_SendCommand(CMD_STOP);

        usleep(to_us(tPausaFin_s)); // Pausa para terminar de recibir todo lo que falte

        // Aviso a la mef que captura los datos que puede exportar el archivo crudo a CSV
        mefSerialToBin_StopReceivingData();

        // Redo
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
