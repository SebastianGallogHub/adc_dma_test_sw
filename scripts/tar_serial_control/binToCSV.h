#ifndef BINTOCSV_H_
#define BINTOCSV_H_

#define TIMESTAMP_FMT "%d%m%y-%H%M%S"

#define BIN_FILE "test-raw_%s"

#define LOG_FILE "test-log_%s.txt"

#define OUTPUT_CHA "test-chA_%s.csv"
#define OUTPUT_CHB "test-chB_%s.csv"

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

void openLogFile();
void closeLogFile();
int writeLogFile(char *buffer, int len);

void openBinFile();
void closeBinFile();
int writeBinFile(char *buffer, int len);
void binToCSV();

#endif // BINTOCSV_H_