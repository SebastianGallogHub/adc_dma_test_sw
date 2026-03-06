#ifndef COMMANDS_H_
#define COMMANDS_H_

#define CMD_HEADER 0x25
#define requires_param(c) c == CMD_CH0_H || c == CMD_CH1_H

typedef enum
{
    CMD_START = 0x01,
    CMD_STOP = 0x02,
    CMD_CH0_H = 0xA0,
    CMD_CH1_H = 0xB0,
    CMD_GET_CONFIG = 0xF0,
} TAR_COMMAND;

#endif // COMMANDS_H_