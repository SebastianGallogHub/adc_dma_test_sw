/***************************** Include Files *******************************/
#include "serial_port.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/************************** Constant Definitions **************************/

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

int serial_ReadByte(char *buffer)
{
    bytes_read = read(serial_fd, buffer, 1);
    return bytes_read;
}

int serial_Init(char *port)
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
