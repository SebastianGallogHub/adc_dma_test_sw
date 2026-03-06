/***************************** Include Files *******************************/
#include "usb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

static libusb_context *ctx = NULL;
static libusb_device_handle *handle = NULL;

/****************************************************************************/

void usb_Close() {
    libusb_release_interface(handle, 0);
    libusb_close(handle);
    libusb_exit(ctx);
}

int  usb_Init() {
    int ret;
    
    printf("Inicializando USB...\n");

    ret = libusb_init(&ctx);
    if (ret < 0) {
        printf("Error libusb_init\n");
        return 1;
    }

    handle = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (!handle) {
        printf("No se pudo abrir el dispositivo USB\n");
        libusb_exit(ctx);
        return 1;
    }

    printf("Dispositivo USB abierto correctamente\n");

    libusb_claim_interface(handle, 0);

    // TODO datos del descriptor del usb
}

void usb_Flush() {
    int transferred = 0;
    int buffer[BUFFER_SIZE];

    // Leo todo lo que haya en un buffer hasta el TIMEOUT 
    // De ese modo se vacía el buffer IN del device
    (int)libusb_bulk_transfer(handle,
                              EP_IN,
                              (unsigned char *)buffer,
                              sizeof(buffer),
                              &transferred,
                              TIMEOUT);
}

void usb_SendCommand(TAR_COMMAND c, ...) {
    int transferred = 0;
    int len = 0;
    uint8_t buffer[16];

    buffer[len++] = (char)CMD_HEADER;
    buffer[len++] = (char)c;

    if (requires_param(c))
    {
        va_list args;
        va_start(args, c);

        uint32_t param = va_arg(args, uint32_t);

        // Enviar el parámetro como 4 bytes
        buffer[len++] = (char)((param >> 24) & 0xFF);
        buffer[len++] = (char)((param >> 16) & 0xFF);
        buffer[len++] = (char)((param >>  8) & 0xFF);
        buffer[len++] = (char)((param >>  0) & 0xFF);
        
        va_end(args);
    }

    (int)libusb_bulk_transfer(handle,
                              EP_OUT,
                              (unsigned char *)buffer,
                              len,
                              &transferred,
                              TIMEOUT);
}

int  usb_ReadBuffer(unsigned char *buffer, int len) {
    int transferred = 0;

    int ret = 0;

    ret = libusb_bulk_transfer(handle,
                               EP_IN,
                               buffer,
                               len,
                               &transferred,
                               TIMEOUT);

    if (ret != 0 && transferred <= 0){
        printf("Error leyendo del dispositivo USB\n");
    }

    return transferred;
}