#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define SECTOR_SIZE     512
#define DATA_SIZE       sizeof(uint32_t)
#define BUFFER_SIZE     (SECTOR_SIZE / DATA_SIZE)

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <dispositivo> <sector_inicial> [cantidad_sectores]\n", argv[0]);
        return 1;
    }

    const char *device = argv[1];
    uint32_t sector_start = atoi(argv[2]);
    uint32_t sector_count = (argc >= 4) ? atoi(argv[3]) : 1;

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("No se pudo abrir el dispositivo");
        return 1;
    }

    uint8_t sector_data[SECTOR_SIZE];

    for (uint32_t s = 0; s < sector_count; s++) {
        off_t offset = (off_t)(sector_start + s) * SECTOR_SIZE;
        if (lseek(fd, offset, SEEK_SET) != offset) {
            perror("Error al hacer seek en el dispositivo");
            close(fd);
            return 1;
        }

        ssize_t read_bytes = read(fd, sector_data, SECTOR_SIZE);
        if (read_bytes != SECTOR_SIZE) {
            perror("Error al leer el sector");
            close(fd);
            return 1;
        }

        printf("Sector %u:\n", sector_start + s);
        for (int i = 0; i < BUFFER_SIZE; i++) {
            uint32_t val;
            val = ((uint32_t)sector_data[i*4 + 0]) |
                  ((uint32_t)sector_data[i*4 + 1] << 8) |
                  ((uint32_t)sector_data[i*4 + 2] << 16) |
                  ((uint32_t)sector_data[i*4 + 3] << 24);
            printf("%u\n", val);
        }
    }

    close(fd);
    return 0;
}
