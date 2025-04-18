#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

int serial_Init(char *port);
void serial_WriteByte(char chr);
int serial_ReadByte(char *buffer);
void serial_Close();

#endif // SERIAL_PORT_H_