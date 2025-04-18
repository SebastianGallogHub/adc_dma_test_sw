/**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>

/**************************************************************************************************/
#define SERIAL_PORT "/dev/ttyUSB1"
#define INPUT_FILE "salida_raw"

#define CMD_END '%'
#define CMD_HEADER_PULSE '&'
#define CMD_FOOTER_PULSE '\''

#define OUTPUT_CH0 "canal_0.csv"
#define OUTPUT_CH1 "canal_1.csv"

#define T_PERIOD 0xFFFFFFFF

#define MSK_HEADER 0xFF00000000000000ULL
#define MSK_TS 0x00FFFFFFFF000000ULL
#define MSK_CH 0x0000000000C00000ULL
#define MSK_VP 0x00000000003FFF00ULL
#define MSK_FOOTER 0x00000000000000FFULL
#define MSK_ALL (MSK_HEADER | MSK_TS | MSK_CH | MSK_VP | MSK_FOOTER)

#define MSK_TS_OFF 24
#define MSK_CH_OFF 22
#define MSK_VP_OFF 8

#define GetFieldFromPulse(pulse, mask, offset) (((pulse) & (mask)) >> (offset))

/**************************************************************************************************/
typedef enum
{
    CONSOLE,
    WAITING_NEXT_PULSE,
    RECEIVING_PULSES,
    CHECK_FALSE_END,
    GENERATING_CSV,
} mef_state;

/**************************************************************************************************/

int serial_init(char *port);
void openInputFile();
void mefSerialToBin(char, int, int);
int writeInputFile(char buffer);
void bin_to_csv();

/**************************************************************************************************/

int serial_fd;
int inputFile_fd;
char buffer;
volatile int keepReceiving = 1;

/**************************************************************************************************/
void *rx_thread_func(void *arg)
{
    int bytes_read;

    openInputFile();

    while (1)
    {
        bytes_read = read(serial_fd, &buffer, 1);

        mefSerialToBin(buffer, bytes_read, keepReceiving);
    }

    return NULL;
}

int main()
{
    // bin_to_csv();

    // return 0;

    if (serial_init("/dev/ttyUSB1"))
        return 1;

    uint8_t command = 0;
    char resp = 0;

    pthread_t rx_thread;
    if (pthread_create(&rx_thread, NULL, rx_thread_func, NULL) != 0)
    {
        perror("No se pudo crear el hilo de recepción");
        close(serial_fd);
        return 1;
    }

    do
    {
        scanf(" %c", &resp);
    } while (resp != 's');

    command = 0x25; // HEADER
    write(serial_fd, &command, 1);
    command = 0x01; // START
    write(serial_fd, &command, 1);

    usleep(60000 * 1000);

    command = 0x25; // HEADER
    write(serial_fd, &command, 1);
    command = 0x02; // STOP
    write(serial_fd, &command, 1);

    usleep(10000 * 1000); // pausa para terminar de recibir todo lo que falte

    keepReceiving = 0;
    pthread_join(rx_thread, NULL);

    close(serial_fd);

    return 0;
}

/**************************************************************************************************/

void openInputFile()
{
    int bytes_captured = 0;

    printf("Capturando datos en %s...\n", INPUT_FILE);
    fflush(stdout);

    // Abrir el archivo de salida
    inputFile_fd = open(INPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

int bytes_captured = 0;
int writeInputFile(char buffer)
{
    if (write(inputFile_fd, &buffer, 1) == -1)
    {
        perror("\nError escribiendo en el archivo\n");
        return 1;
    }

    bytes_captured++;

    if (bytes_captured % 80 == 0)
    {
        printf(".");
        fflush(stdout);
    }
    return 0;
}

void mefSerialToBin(char buffer, int bytes_read, int keepReceiving)
{
    static mef_state state = CONSOLE;

    switch (state)
    {
    case CONSOLE:

        if (bytes_read)
        {
            if (buffer == CMD_FOOTER_PULSE) // Los datos vienen al revés!!!! BIGENDIAN
            {
                writeInputFile(buffer);
                state = RECEIVING_PULSES;
            }
            else
            {
                printf("%c", buffer);
                fflush(stdout);
            }
        }

        break;

    case RECEIVING_PULSES:
        if (keepReceiving)
        {
            if (bytes_read)
            {
                if (writeInputFile(buffer))
                    break;
            }
        }
        else
        {
            state = GENERATING_CSV;
        }

        break;

    case GENERATING_CSV:
        printf("\n");
        bin_to_csv();

        state = CONSOLE;

        break;

    default:
        state = CONSOLE;
        break;
    }
}

void bin_to_csv()
{
    FILE *input = fopen(INPUT_FILE, "rb");
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
    printf("Index\tChannel\tTimestamp\tValue\n");

    uint8_t buffer[8];
    uint64_t pulse;
    size_t index_ch0 = 0, index_ch1 = 0, ii = 0, of_count = 0;
    uint8_t ch = 0;
    uint32_t ts = 0;
    uint16_t vp = 0;
    uint64_t time = 0;

    // while (fread(&pulse, sizeof(uint64_t), 1, input) == 1)
    while (fread(&buffer, 1, 8, input) == 8)
    {
        if (buffer[7] == 0)
            fread(&(buffer[7]), 1, 1, input);

        pulse = 0;
        for (int i = 7; i >= 0; i--) // Los datos vienen al revés BIGENDIAN
            pulse |= ((uint64_t)buffer[i]) << (8 * i);

        ch = GetFieldFromPulse(pulse, MSK_CH, MSK_CH_OFF);
        ts = GetFieldFromPulse(pulse, MSK_TS, MSK_TS_OFF);
        vp = GetFieldFromPulse(pulse, MSK_VP, MSK_VP_OFF);
        if (ii == 59)
            ii = 59;

        printf("%lu\t%u\t%lu\t\t%u\t\t(0x%x)\n", ii++, ch, ts, vp, (unsigned int)pulse);

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
}

int serial_init(char *port)
{
    struct termios options;

    serial_fd = open(port, O_RDWR | O_NOCTTY);
    if (serial_fd == -1)
    {
        perror("Error abriendo el puerto serie");
        return 1;
    }

    // Configurar el puerto serie
    tcgetattr(serial_fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag &= ~PARENB; // Sin paridad
    options.c_cflag &= ~CSTOPB; // 1 bit de stop
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;                             // 8 bits de datos
    options.c_cflag |= CREAD | CLOCAL;                  // Habilitar recepción
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Modo raw
    options.c_oflag &= ~OPOST;

    tcsetattr(serial_fd, TCSANOW, &options);

    printf("Escuchando puerto serie\n");

    return 0;
}
