#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

typedef enum
{
    CMD_HEADER = 0x25,
    CMD_START = 0x01,
    CMD_STOP = 0x02,
    CMD_CH0_H = 0xA0,
    CMD_CH1_H = 0xB0,
} SERIAL_COMMAND;

int serial_Init(char *port);
void serial_WriteByte(char chr);
void serial_Flush();
void serial_SendCommand(SERIAL_COMMAND c, ...);
int serial_ReadByte(char *buffer);
void serial_Close();

#endif // SERIAL_PORT_H_