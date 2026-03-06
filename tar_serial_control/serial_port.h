#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#include "commands.h"

int serial_Init(char *port);
void serial_Flush();
void serial_SendCommand(TAR_COMMAND c, ...);
void serial_Close();
int serial_ReadBuffer(char *buffer, int len, int idle_timeout_ms);

#endif // SERIAL_PORT_H_