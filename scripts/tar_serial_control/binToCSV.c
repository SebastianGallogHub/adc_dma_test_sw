/***************************** Include Files *******************************/
#include "binToCSV.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
int binFile_fd = 0;
int bytes_captured = 0;
int terminalBreak = 0;

/****************************************************************************/
void openBinFile()
{
    int bytes_captured = 0;

    printf("Capturando datos en %s...\n", BIN_FILE);
    fflush(stdout);

    // Abrir el archivo de salida
    binFile_fd = open(BIN_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

void closeBinFile()
{
    close(binFile_fd);
}

int writeBinFile(char buffer)
{
    if (write(binFile_fd, &buffer, 1) == -1)
    {
        perror("\nError escribiendo en el archivo\n");
        return 1;
    }

    bytes_captured++;

    if (bytes_captured % 80 == 0)
    {
        printf(".");
        // fflush(stdout);
    }
    return 0;
}

void binToCSV()
{
    uint8_t buffer[8];
    uint64_t pulse;
    size_t index_ch0 = 0, index_ch1 = 0, ii = 0, of_count = 0;
    uint8_t ch = 0;
    uint32_t ts = 0;
    uint16_t vp = 0;
    uint64_t time = 0;

    FILE *input = fopen(BIN_FILE, "rb");
    if (!input)
    {
        perror("Error al abrir archivo binario de entrada");
        return;
    }

    FILE *output_ch0 = fopen(OUTPUT_CH0, "w");
    FILE *output_ch1 = fopen(OUTPUT_CH1, "w");
    if (!output_ch0 || !output_ch1)
    {
        perror("Error al abrir archivo de salida");
        fclose(input);
        if (output_ch0)
            fclose(output_ch0);
        if (output_ch1)
            fclose(output_ch1);
        return;
    }

    fprintf(output_ch0, "Index,Timestamp,Value\n");
    fprintf(output_ch1, "Index,Timestamp,Value\n");

    // printf("Index\tChannel\tTimestamp\tValue\n");

    while (fread(&buffer, 1, 8, input) == 8)
    {
        pulse = 0;
        for (int i = 7; i >= 0; i--) // Los datos vienen al revés BIGENDIAN
            pulse |= ((uint64_t)buffer[i]) << (8 * i);

        ch = GetFieldFromPulse(pulse, MSK_CH, MSK_CH_OFF);
        ts = GetFieldFromPulse(pulse, MSK_TS, MSK_TS_OFF);
        vp = GetFieldFromPulse(pulse, MSK_VP, MSK_VP_OFF);

        // printf("%lu\t%u\t%u\t\t%u\t\t(0x%x)\n", ii++, ch, ts, vp, (unsigned int)pulse);

        switch (ch)
        {
        case 1:
            fprintf(output_ch0, "%zu,%lu,%u\n", index_ch0++, time + ts, vp);
            break;

        case 2:
            fprintf(output_ch1, "%zu,%lu,%u\n", index_ch1++, time + ts, vp);
            break;

        case 3:
            of_count++;
            time += (uint64_t)T_PERIOD;
            break;

        default:
            break;
        }
    }

    fclose(input);
    fclose(output_ch0);
    fclose(output_ch1);

    printf("Se capturaron %d bytes (%d pulsos)\n", bytes_captured, bytes_captured / 8);
    printf("Se capturaron %lu marcas de tiempo\n", of_count);
    printf("Conversión completa:\n\t - Canal 0: %s (%lu)\n\t - Canal 1: %s (%lu)\n", OUTPUT_CH0, index_ch0, OUTPUT_CH1, index_ch1);

    bytes_captured = 0;
}