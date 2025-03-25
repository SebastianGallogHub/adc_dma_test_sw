#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#define SERIAL_PORT "/dev/ttyUSB1"
#define OUTPUT_FILE "salida_raw"
#define MAX_BYTES 2000

int main() {
    int serial_fd, file_fd;
    struct termios options;
    char buffer;
    int bytes_captured = 0;

    // Abrir el puerto serie
    serial_fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    if (serial_fd == -1) {
        perror("Error abriendo el puerto serie");
        return 1;
    }

    printf("Capturando datos...\n");

    // Configurar el puerto serie
    tcgetattr(serial_fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag &= ~PARENB;  // Sin paridad
    options.c_cflag &= ~CSTOPB;  // 1 bit de stop
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;      // 8 bits de datos
    options.c_cflag |= CREAD | CLOCAL; // Habilitar recepción
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Modo raw
    options.c_oflag &= ~OPOST;

    tcsetattr(serial_fd, TCSANOW, &options);

    // Abrir el archivo de salida
    file_fd = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd == -1) {
        perror("Error abriendo el archivo de salida");
        close(serial_fd);
        return 1;
    }

    // Capturar datos hasta alcanzar 2000 bytes o recibir '\n'
    while (bytes_captured < MAX_BYTES) {
        int bytes_read = read(serial_fd, &buffer, 1);

        if (bytes_read > 0) {
            // Escribir el byte en el archivo
            if (write(file_fd, &buffer, 1) == -1) {
                perror("Error escribiendo en el archivo");
                break;
            }
            bytes_captured++;
            
            if(bytes_captured%40 == 0)
            	printf(".");

            // Detenerse si llega '\n'
            //if (buffer == '\n') {
            //    break;
            //}
        } /* else if (bytes_read == -1 && errno != EINTR) {
            perror("Error leyendo desde el puerto serie");
            break;
        }*/
    }

    // Cerrar archivos
    close(serial_fd);
    close(file_fd);

    printf("Fin. Datos capturados: %d bytes\n", bytes_captured);
    return 0;
}

