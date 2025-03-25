#include <stdio.h>
#include <stdlib.h>

#define SERIAL_CAPTURE "./serial_capture"
#define BIN_TO_CSV "./bin_to_csv"

int main() {
    int status;

    // Ejecutar serial_capture
    printf("Iniciando captura de datos...\n");
    status = system(SERIAL_CAPTURE);
    if (status != 0) {
        fprintf(stderr, "Error ejecutando %s\n", SERIAL_CAPTURE);
        return 1;
    }

    // Ejecutar bin_to_csv
    printf("Convirtiendo datos a CSV...\n");
    status = system(BIN_TO_CSV);
    if (status != 0) {
        fprintf(stderr, "Error ejecutando %s\n", BIN_TO_CSV);
        return 1;
    }

    printf("Proceso completado exitosamente.\n");
    return 0;
}

