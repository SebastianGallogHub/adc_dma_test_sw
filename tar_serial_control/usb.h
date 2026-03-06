#ifndef USB_H_
#define USB_H_

#define VID 0xFE1A
#define PID 0x0001

#define EP_OUT 0x01
#define EP_IN  0x81

#define TIMEOUT 1000
#define BUFFER_SIZE 512 * 16

#include "commands.h"

int  usb_Init();
void usb_Flush();
void usb_SendCommand(TAR_COMMAND c, ...);
void usb_Close();
int  usb_ReadBuffer(unsigned char *buffer, int len);
// int  usb_ReadBuffer(char *buffer, int len, int idle_timeout_ms);

#endif // USB_H