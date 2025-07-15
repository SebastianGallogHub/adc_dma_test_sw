/***************************** Include Files *******************************/
#include <stdio.h>
#include <string.h>
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
#define to_hist(low, high) ((uint32_t)high) << 16 | (uint16_t)low
#define to_us(s) s * 1000 /*ms*/ * 1000 /*us*/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
void print_help(const char *prog_name);

void sendHysteresis(uint16_t hist0_low, uint16_t hist0_high, uint16_t hist1_low, uint16_t hist1_high);
int readLOG();

void *serialToBin_thread_func(void *arg);

/************************** Variable Definitions ***************************/
int rx_thread_stop = 0;

/****************************************************************************/

int main(int argc, char *argv[])
{
    char *puerto = NULL;
    uint16_t hist0_low = 0, hist0_high = 0;
    uint16_t hist1_low = 0, hist1_high = 0;
    int tEnsayo_s = 0;

    // Parseo de argumentos
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_help(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-ha") == 0 && i + 2 < argc)
        {
            hist0_low = atoi(argv[++i]);
            hist0_high = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-hb") == 0 && i + 2 < argc)
        {
            hist1_low = atoi(argv[++i]);
            hist1_high = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
        {
            tEnsayo_s = atoi(argv[++i]);
        }
        else if (argv[i][0] != '-')
        {
            puerto = argv[i];
        }
    }

    if (!puerto || tEnsayo_s <= 0)
    {
        fprintf(stderr, "Error: argumentos inválidos o incompletos.\n");
        print_help(argv[0]);
        return 1;
    }

    int bytes_read;
    char buffer[1024];
    pthread_t rx_thread;

    if (serial_Init(puerto))
        return 1;

    // do
    // {
    //     i = 0;
    //     j = 0;
    //     log = 0;
    //     rx_thread_stop = 0;

    // Limpio el buffer Tx del micro
    bytes_read = serial_ReadBuffer(buffer, sizeof(buffer), 1000);

    sendHysteresis(hist0_low, hist0_high, hist1_low, hist1_high);

    if (readLOG())
        return 1;

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

    //     printf("\n");
    //     printf("Repeat?\n");
    //     scanf(" %c", &resp);
    // }
    // while (resp == 's' || resp == 'S')
    //     ;

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

void sendHysteresis(uint16_t hist0_low, uint16_t hist0_high, uint16_t hist1_low, uint16_t hist1_high)
{
    // Envío la configuración de histéresis
    printf("-> Enviando histéresis\n");

    if (hist0_high != 0 && hist0_low != 0)
    {
        printf("\t-> CHA: histéresis (%d ; %d)mV -> (%d ; %d)\n", hist0_low, hist0_high, to_cad(hist0_low), to_cad(hist0_high));
        fflush(stdout);
        serial_SendCommand(CMD_CH0_H, to_hist(to_cad(hist0_low), to_cad(hist0_high)));
    }
    else
    {
        printf("\t-> CHA: DESHABILITADO\n");
        fflush(stdout);
    }

    usleep(1000000);

    if (hist1_high != 0 && hist1_low != 0)
    {
        printf("\t-> CHB: histéresis (%d ; %d)mV -> (%d ; %d)\n", hist1_low, hist1_high, to_cad(hist1_low), to_cad(hist1_high));
        fflush(stdout);
        serial_SendCommand(CMD_CH1_H, to_hist(to_cad(hist1_low), to_cad(hist1_high)));
    }
    else
    {
        printf("\t-> CHB: DESHABILITADO\n");
        fflush(stdout);
    }

    usleep(1000000);
}

int readLOG()
{
    int i = 0, j = 0;
    int log = 0;
    int bytes_read;
    char buffer[1024];

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
        printf("\nERROR LEYENDO LOG\n");
        serial_Close();
        return 1;
    }
}

void print_help(const char *prog_name)
{
    printf("Uso:\n");
    printf("  %s <puerto> -t <tiempo_s> [-ha <lowA> <highA>] [-hb <lowB> <highB>]\n ", prog_name);
    printf("\nEjemplo:\n");
    printf("  %s /dev/ttyUSB1 -t 1 -ha 1000 3000 -hb 1000 3000\n", prog_name);
    printf("\nOpciones:\n");
    printf("  <puerto>         Dispositivo serie, por ejemplo /dev/ttyUSB1\n");
    printf("  -t  tiempo_s     Tiempo de adquisición en segundos\n");
    printf("  -ha low high     Histéresis canal A (opcional, si no se configura se deshabilita el hardware)\n");
    printf("  -hb low high     Histéresis canal B (opcional, si no se configura se deshabilita el hardware)\n");
    printf("  -h, --help       Muestra esta ayuda y termina\n");
}
/**************************************************************************************************/
