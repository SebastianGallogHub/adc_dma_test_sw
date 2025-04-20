/***************************** Include Files *******************************/
#include "serial_port.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/************************** Constant Definitions **************************/
#define requires_param(c) c == CMD_CH0_H || c == CMD_CH1_H

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
int serial_fd;
int bytes_read;

/****************************************************************************/

void serial_Close()
{
    close(serial_fd);
}

void serial_WriteByte(char chr)
{
    write(serial_fd, &chr, 1);
}

void serial_Flush()
{
    tcflush(serial_fd, TCIOFLUSH); // Limpia el buffer
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

    usleep(1000);
}

int serial_ReadByte(char *buffer)
{
    bytes_read = read(serial_fd, buffer, 1);
    return bytes_read;
}

int serial_Init(char *port)
{
    struct termios options;

    serial_fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK); // No bloqueante

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
    options.c_cflag |= CS8;            // 8 bits de datos
    options.c_cflag |= CREAD | CLOCAL; // Habilitar recepción

    options.c_iflag &= ~(IXON | IXOFF | IXANY);           // Desactiva control de flujo por software
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR | ISTRIP); // No convierte \n o \r. ISTRIP desactiva el stripping de 8 bits

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Modo raw

    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 0;  // read() no espera ningún byte
    options.c_cc[VTIME] = 0; // read() devuelve 0 si no hay datos de inmediato

    tcsetattr(serial_fd, TCSANOW, &options);

    serial_Flush();

    printf("Escuchando puerto serie\n");

    return 0;
}
