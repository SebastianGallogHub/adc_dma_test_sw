#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#define SERIAL_PORT "/dev/ttyUSB1"
// #define SERIAL_PORT "/dev/ttyUSB"
#define INPUT_FILE "salida_raw"
#define OUTPUT_FILE "salida.csv"
#define CHR_SIGN '&'

int serial_fd;
char buffer;

void serial_to_bin();
void bin_to_csv();

int main()
{
    struct termios options;
    int bytes_read;
    // Abrir el puerto serie
    // for (int i; i < 5; i++)
    // {
    //     serial_fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    //     if (serial_fd == -1)
    //     {
    //         perror("Error abriendo el puerto serie");
    //         // return 1;
    //     }
    // }

    serial_fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    if (serial_fd == -1)
    {
        perror("Error abriendo el puerto serie");
        return 1;
    }

    // if (serial_fd < 0)
    // {
    //     return 1;
    // }

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

    do
    {
        bytes_read = read(serial_fd, &buffer, 1);

        if (bytes_read > 0)
        {
            if (buffer == CHR_SIGN)
            {
                serial_to_bin();
            }
            else
            {
                printf("%c", buffer);
                fflush(stdout);
            }
        }
    } while (1);

    return 0;
}

void serial_to_bin()
{
    int file_fd;
    int bytes_captured = 0;

    printf("Capturando datos en %s...\n", INPUT_FILE);
    fflush(stdout);

    // Abrir el archivo de salida
    file_fd = open(INPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd == -1)
    {
        perror("Error abriendo el archivo de salida");
        close(serial_fd);
        return;
    }

    while (1)
    {
        int bytes_read = read(serial_fd, &buffer, 1);

        if (bytes_read > 0)
        {
            // Detenerse si llega '%'
            if (buffer == CHR_SIGN)
            {
                break;
            }

            // Escribir el byte en el archivo
            if (write(file_fd, &buffer, 1) == -1)
            {
                perror("Error escribiendo en el archivo");
                break;
            }

            bytes_captured++;

            if (bytes_captured % 40 == 0)
            {
                printf(".");
                fflush(stdout);
            }
        }
    }

    // Cerrar archivos
    close(file_fd);

    printf("\nFin. Datos capturados: %d bytes\n", bytes_captured);
    fflush(stdout);

    bin_to_csv();
}

void bin_to_csv()
{
    FILE *input, *output;
    uint8_t buffer[2]; // Para almacenar dos bytes (16 bits)
    uint16_t data;
    size_t bytes_read;
    int channel = 0;    // Alternador de canal (0 = CH1, 1 = CH2)
    int row_number = 1; // Contador de filas

    // Abrir el archivo binario de entrada
    input = fopen(INPUT_FILE, "rb");
    if (!input)
    {
        perror("Error abriendo el archivo de entrada");
        return;
    }

    // Abrir el archivo CSV de salida
    output = fopen(OUTPUT_FILE, "w");
    if (!output)
    {
        perror("Error abriendo el archivo de salida");
        fclose(input);
        return;
    }

    // Escribir encabezado en CSV
    fprintf(output, "Fila,CH1,CH2\n");

    uint16_t ch1 = 0, ch2 = 0;

    // Leer datos de 16 bits y escribir en CSV alternando entre CH1 y CH2
    while ((bytes_read = fread(buffer, 1, 2, input)) == 2)
    {
        // Convertir bytes a un valor de 16 bits (little-endian)
        data = (buffer[1] << 8) | buffer[0];

        if (channel == 0)
        {
            ch2 = data;  // Guardar en CH1
            channel = 1; // Cambiar a CH2
        }
        else
        {
            ch1 = data; // Guardar en CH2
            fprintf(output, "%d,%u,%u\n", row_number++, ch1, ch2);
            channel = 0; // Volver a CH1
        }
    }

    // Si el número de datos es impar, escribir el último valor en CH1 sin CH2
    if (channel == 1)
    {
        fprintf(output, "%d,%u,\n", row_number, ch1);
    }

    // Cerrar archivos
    fclose(input);
    fclose(output);

    printf("Conversión completada. Datos guardados en %s\n", OUTPUT_FILE);
    fflush(stdout);
    //    return 0;
}
