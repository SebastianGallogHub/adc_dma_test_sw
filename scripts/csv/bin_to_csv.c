#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define INPUT_FILE "salida_raw"
#define OUTPUT_FILE "salida.csv"

int main() {
    FILE *input, *output;
    uint8_t buffer[2]; // Para almacenar dos bytes (16 bits)
    uint16_t data;
    size_t bytes_read;
    int channel = 0; // Alternador de canal (0 = CH1, 1 = CH2)
    int row_number = 1; // Contador de filas

    // Abrir el archivo binario de entrada
    input = fopen(INPUT_FILE, "rb");
    if (!input) {
        perror("Error abriendo el archivo de entrada");
        return 1;
    }

    // Abrir el archivo CSV de salida
    output = fopen(OUTPUT_FILE, "w");
    if (!output) {
        perror("Error abriendo el archivo de salida");
        fclose(input);
        return 1;
    }

    // Escribir encabezado en CSV
    fprintf(output, "Fila,CH1,CH2\n");

    uint16_t ch1 = 0, ch2 = 0;

    // Leer datos de 16 bits y escribir en CSV alternando entre CH1 y CH2
    while ((bytes_read = fread(buffer, 1, 2, input)) == 2) {
        // Convertir bytes a un valor de 16 bits (little-endian)
        data = (buffer[1] << 8) | buffer[0];

        if (channel == 0) {
            ch2 = data; // Guardar en CH1
            channel = 1; // Cambiar a CH2
        } else {
            ch1 = data; // Guardar en CH2
            fprintf(output, "%d,%u,%u\n", row_number++, ch1, ch2);
            channel = 0; // Volver a CH1
        }
    }

    // Si el número de datos es impar, escribir el último valor en CH1 sin CH2
    if (channel == 1) {
        fprintf(output, "%d,%u,\n", row_number, ch1);
    }

    // Cerrar archivos
    fclose(input);
    fclose(output);

    printf("Conversión completada. Datos guardados en %s\n", OUTPUT_FILE);
    return 0;
}

