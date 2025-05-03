/***************************** Include Files *******************************/
#include "mefSerialToBin.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "binToCSV.h"

/************************** Constant Definitions **************************/
#define CMD_CONFIG_MARK '%'
#define CMD_HEADER_PULSE '&'
#define CMD_FOOTER_PULSE '\''

/**************************** Type Definitions ******************************/
typedef enum
{
    IDLE,
    CONFIG,
    RECEIVING_PULSES,
    GENERATE_CSV,
} MEF_STATE;

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
int keepReceiving = 0;
int configReceived = 0;

/****************************************************************************/

void mefSerialToBin_Reset()
{
    configReceived = 0;
}

void mefSerialToBin_StopReceivingData()
{
    keepReceiving = 0;
}

void mefSerialToBin_StartReceivingData()
{
    keepReceiving = 1;
    openBinFile();
}

int mefSerialToBin_ConfigReceived()
{
    return configReceived;
}

void mefSerialToBin(char buffer, int bytes_read)
{
    static MEF_STATE state = IDLE;

    switch (state)
    {
    case IDLE:
        if (bytes_read > 0)
        {
            if (buffer == CMD_CONFIG_MARK)
                state = CONFIG;
            else if (buffer == CMD_FOOTER_PULSE && keepReceiving) // Los datos vienen al revés!!!! BIGENDIAN
            {
                writeBinFile(buffer);
                state = RECEIVING_PULSES;
            }
            else
            {
                printf("%c", buffer);
                fflush(stdout);
            }
        }

        break;

    case CONFIG:
        if (bytes_read > 0)
        {
            if (buffer == CMD_CONFIG_MARK)
            {
                configReceived = 1;
                state = IDLE;
            }
            else
            {
                // todo Escribir el archivo de configuraciones
                printf("%c", buffer);
                fflush(stdout);
            }
        }
        break;

    case RECEIVING_PULSES:
        if (bytes_read > 0 && keepReceiving)
        {
            if (writeBinFile(buffer))
                break;
        }

        if (!keepReceiving)
        {
            closeBinFile();
            state = GENERATE_CSV;
        }

        break;

    case GENERATE_CSV:
        printf("\n");
        binToCSV();

        state = IDLE;

        break;

    default:
        state = IDLE;
        break;
    }
}
