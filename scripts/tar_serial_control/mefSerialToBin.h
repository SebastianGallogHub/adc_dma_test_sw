#ifndef MEF_SERIAL_TO_BIN_H_
#define MEF_SERIAL_TO_BIN_H_

void mefSerialToBin_Reset();
void mefSerialToBin(char buffer, int bytes_read);
void mefSerialToBin_StartReceivingData();
void mefSerialToBin_StopReceivingData();
int mefSerialToBin_ConfigReceived();

#endif // MEF_SERIAL_TO_BIN_H_