/***************************** Include Files *******************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "serial_port.h"
#include "binToCSV.h"

/************************** Constant Definitions **************************/
#define ZMODADC1410_RESOLUTION 3.21f /*mv*/
#define to_cad(mv) (uint16_t)(mv / ZMODADC1410_RESOLUTION)
#define to_hist(low, high) ((uint32_t)high) << 16 | low
#define to_us(s) s * 1000 /*ms*/ * 1000 /*us*/

/**************************** Type Definitions ******************************/
typedef enum
{
    SEND_HYSTERESIS,
    GET_CONFIG_LOG,
    START_TAR,
    RECEIVING_DATA,
    STOP_TAR,
    AWAIT_DATA,
    REPEAT,
} MEF_TAR_CONTROL_STATE;

/************************** Function Prototypes *****************************/
void *serialToBin_thread_func(void *arg);

/************************** Variable Definitions ***************************/
int rx_thread_stop = 0;
/****************************************************************************/

int main()
{
    char resp = 0;
    int tEnsayo_s = 1;

    uint16_t hist0_low = 1000, hist0_high = 3000;
    uint16_t hist1_low = 1000, hist1_high = 3000;

    int i = 0, j = 0;
    int log = 0;
    int bytes_read;
    char buffer[1024];
    pthread_t rx_thread;

    if (serial_Init("/dev/ttyUSB1"))
        return 1;

    do
    {
        i = 0;
        j = 0;
        log = 0;
        rx_thread_stop = 0;

        // Limpio el buffer Tx del micro
        bytes_read = serial_ReadBuffer(buffer, sizeof(buffer), 1000);

        // Envío la configuración de histéresis
        printf("-> Enviando histéresis\n");
        serial_SendCommand(CMD_CH0_H, to_hist(to_cad(hist0_low), to_cad(hist0_high)));
        usleep(1000000);
        serial_SendCommand(CMD_CH1_H, to_hist(to_cad(hist1_low), to_cad(hist1_high)));
        usleep(1000000);

        // Leo el log del sistema
        printf("-> Leyendo LOG\n");
        serial_SendCommand(CMD_GET_CONFIG);

        bytes_read = serial_ReadBuffer(buffer, sizeof(buffer), 1000);

        if (bytes_read > 0)
        {
            while (1)
            {
                if (log == 0)
                {
                    if (buffer[i++] == '{')
                    {
                        log = 1;
                        j += i;
                    }
                }
                else
                {
                    if (buffer[j++] == '}')
                    {
                        j--;
                        j--;
                        break;
                    }
                }
            }

            openLogFile();
            writeLogFile(buffer + i, j);
            closeLogFile();

            fwrite(buffer + i, 1, j, stdout);
            printf("\n");
            fflush(stdout);
        }
        else
        {
            printf("NO LEYÓ LOG\n");
            serial_Close();
            return 1;
        }

        // Abro el archivo para que esté preparado
        openBinFile();

        // Evío comando de start
        printf("-> Iniciando medición: %d s\n", tEnsayo_s);
        serial_SendCommand(CMD_START);

        // Creo el hilo de lectura
        if (pthread_create(&rx_thread, NULL, serialToBin_thread_func, NULL) != 0)
        {
            perror("No se pudo crear el hilo de recepción");
            serial_SendCommand(CMD_STOP);
            serial_Close();
            return 1;
        }

        // Se detiene el hilo para recibir datos
        usleep(to_us(tEnsayo_s));

        // Envío comando de stop
        serial_SendCommand(CMD_STOP);
        rx_thread_stop = 1;

        // Espero a que el hilo de lectura termine
        pthread_join(rx_thread, NULL);

        // Cierro el archivo binario
        closeBinFile();

        // Convierto a CSV
        binToCSV();

        printf("\n");

        serial_SendCommand(CMD_GET_CONFIG);
        bytes_read = serial_ReadBuffer(buffer, sizeof(buffer), 3000);
        if (bytes_read > 0)
        {
            fwrite(buffer, 1, bytes_read, stdout);
        }

        printf("Repeat?\n");
        scanf(" %c", &resp);
    } while (resp == 's' || resp == 'S');

    serial_Close();

    return 0;
}

void *serialToBin_thread_func(void *arg)
{
    char buffer[800];
    int bytes_read;

    while (1)
    {
        bytes_read = serial_ReadBuffer(buffer, sizeof(buffer), 1000);

        if (bytes_read > 0)
            writeBinFile(buffer, bytes_read);
        else if (rx_thread_stop)
            break;
    }

    return NULL;
}

/**************************************************************************************************/
