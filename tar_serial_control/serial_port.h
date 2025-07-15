#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#define CMD_HEADER 0x25

typedef enum
{
    CMD_START = 0x01,
    CMD_STOP = 0x02,
    CMD_CH0_H = 0xA0,
    CMD_CH1_H = 0xB0,
    CMD_GET_CONFIG = 0xF0,
} SERIAL_COMMAND;

int serial_Init(char *port);
void serial_WriteByte(char chr);
void serial_Flush();
void serial_SendCommand(SERIAL_COMMAND c, ...);
int serial_ReadByte(char *buffer);
void serial_Close();
int serial_ReadBuffer(char *buffer, int len, int idle_timeout_ms);

#endif // SERIAL_PORT_H_