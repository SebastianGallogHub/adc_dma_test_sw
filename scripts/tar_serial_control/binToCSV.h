#ifndef BINTOCSV_H_
#define BINTOCSV_H_

#define BIN_FILE "salida_raw"

#define OUTPUT_CH0 "canal_0.csv"
#define OUTPUT_CH1 "canal_1.csv"

#define T_PERIOD 0xFFFFFFFF

#define MSK_HEADER 0xFF00000000000000ULL
#define MSK_TS 0x00FFFFFFFF000000ULL
#define MSK_CH 0x0000000000C00000ULL
#define MSK_VP 0x00000000003FFF00ULL
#define MSK_FOOTER 0x00000000000000FFULL
#define MSK_ALL (MSK_HEADER | MSK_TS | MSK_CH | MSK_VP | MSK_FOOTER)

#define MSK_TS_OFF 24
#define MSK_CH_OFF 22
#define MSK_VP_OFF 8

#define GetFieldFromPulse(pulse, mask, offset) (((pulse) & (mask)) >> (offset))

void openBinFile();
void closeBinFile();
int writeBinFile(char buffer);
void binToCSV();

#endif // BINTOCSV_H_