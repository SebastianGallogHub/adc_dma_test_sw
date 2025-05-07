/***************************** Include Files *******************************/
#include "binToCSV.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
int logFile_fd = 0;
int binFile_fd = 0;
int bytes_captured = 0;
int terminalBreak = 0;

char timestamp[32];

/****************************************************************************/
void openLogFile()
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[256];

    // Guardar el timestamp en una variable reutilizable
    strftime(timestamp, sizeof(timestamp), TIMESTAMP_FMT, t);

    // Usar timestamp para varios archivos
    snprintf(filename, sizeof(filename), LOG_FILE, timestamp);

    // Abrir el archivo de log
    logFile_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

void closeLogFile()
{
    close(logFile_fd);
}

int writeLogFile(char *buffer, int len)
{
    if (write(logFile_fd, buffer, len) == -1)
    {
        perror("\nError escribiendo en el archivo log\n");
        return 1;
    }
}

/****************************************************************************/
void openBinFile()
{
    char filename[256];

    bytes_captured = 0;

    // Usar timestamp para varios archivos
    snprintf(filename, sizeof(filename), BIN_FILE, timestamp);

    printf("Capturando datos en %s\n", filename);
    fflush(stdout);

    // Abrir el archivo de salida
    binFile_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

void closeBinFile()
{
    close(binFile_fd);
}

int writeBinFile(char *buffer, int len)
{
    if (write(binFile_fd, buffer, len) == -1)
    {
        perror("\nError escribiendo en el archivo crudo\n");
        return 1;
    }

    bytes_captured += len;

    // if (bytes_captured % 80 == 0)
    // {
    printf(".");
    fflush(stdout);
    // }

    return 0;
}

void binToCSV()
{
    uint8_t buffer[8];
    uint64_t pulse;
    size_t index_chA = 0, index_chB = 0, ii = 0, of_count = 0;
    uint8_t ch = 0;
    uint32_t ts = 0;
    uint16_t vp = 0;
    uint64_t time = 0;

    char binFilename[256];
    char outAFilename[256];
    char outBFilename[256];
    snprintf(binFilename, sizeof(binFilename), BIN_FILE, timestamp);
    snprintf(outAFilename, sizeof(outAFilename), OUTPUT_CHA, timestamp);
    snprintf(outBFilename, sizeof(outBFilename), OUTPUT_CHB, timestamp);

    printf("Se capturaron %d bytes (%d pulsos)\n", bytes_captured, bytes_captured / 8);
    printf("Convirtiendo archivo binario -> CSV\n");
    fflush(stdout);

    FILE *input = fopen(binFilename, "rb");
    if (!input)
    {
        perror("Error al abrir archivo binario de entrada");
        return;
    }

    FILE *output_chA = fopen(outAFilename, "w");
    FILE *output_chB = fopen(outBFilename, "w");
    if (!output_chA || !output_chB)
    {
        perror("Error al abrir archivo de salida");
        fclose(input);
        if (output_chA)
            fclose(output_chA);
        if (output_chB)
            fclose(output_chB);
        return;
    }

    fprintf(output_chA, "Index,Timestamp,Value\n");
    fprintf(output_chB, "Index,Timestamp,Value\n");

    // printf("Index\tChannel\tTimestamp\tValue\n");

    while (fread(&buffer, 1, 8, input) == 8)
    {
        pulse = 0;
        for (int i = 7; i >= 0; i--) // Los datos vienen al revés BIGENDIAN
            pulse |= ((uint64_t)buffer[i]) << (8 * i);

        ch = GetFieldFromPulse(pulse, MSK_CH, MSK_CH_OFF);
        ts = GetFieldFromPulse(pulse, MSK_TS, MSK_TS_OFF);
        vp = GetFieldFromPulse(pulse, MSK_VP, MSK_VP_OFF);

        // printf("%lu\t%u\t%u\t\t%u\t\t(0x%x)\n", ii, ch, ts, vp, (unsigned int)pulse);

        if (ii++ % 800 == 0)
        {
            printf(".");
            fflush(stdout);
        }

        switch (ch)
        {
        case 1:
            fprintf(output_chA, "%zu,%lu,%u\n", index_chA++, time + ts, vp);
            break;

        case 2:
            fprintf(output_chB, "%zu,%lu,%u\n", index_chB++, time + ts, vp);
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
    fclose(output_chA);
    fclose(output_chB);

    printf("\nSe reconocieron %lu marcas de tiempo\n", of_count);
    printf("Conversión completa:\n\tCHA: %s (%lu pulsos)\n\tCHB: %s (%lu pulsos)\n", outAFilename, index_chA, outBFilename, index_chB);
    fflush(stdout);

    bytes_captured = 0;
}