#ifndef USB_H_
#define USB_H_

#define VID 0xFE1A
#define PID 0x0001

#define EP_OUT 0x01
#define EP_IN  0x81

#define USB_TIMEOUT 1000
#define USB_BUFFER_SIZE 512 * 16

#define USB_ERR_IO              -1
#define USB_ERR_NOT_FOUND       -5
#define USB_ERR_TIMEOUT         -7
#define USB_ERR_PIPE            -9
#define USB_ERR_NOT_SUPPORTED   -12


#include "commands.h"

int  usb_Init();
void usb_Flush();
void usb_SendCommand(TAR_COMMAND c, ...);
void usb_Close();
int  usb_ReadBuffer(unsigned char *buffer, int len, unsigned int timeout_ms);

#endif // USB_H