#ifndef APP_GLOBAL_H_
#define APP_GLOBAL_H_

#include <ti/drivers/UART.h>

#define _u8 unsigned char
#define _u16 unsigned short
#define _u32 unsigned int
#define _u64 unsigned long
#define _i8 signed char

#define ERROR_ProcessBridgePacket1      1
#define ERROR_RadioTaskSendPacket1      2
#define ERROR_RadioTaskSendPacket2      3
#define ERROR_RadioTaskSendPacket3      4
#define ERROR_RadioTaskSendPacket4      5
#define ERROR_RadioTaskSendPacket5      6
#define ERROR_ProcessBridgePacket2      7
#define ERROR_ProcessBridgePacket3      8
#define ERROR_ProcessBridgePacket4      9
#define ERROR_ProcessBridgePacket5      10
#define ERROR_ProcessBridgePacket6      11
#define ERROR_ProcessBridgePacket7      12
#define ERROR_ProcessBridgePacket8      13
#define ERROR_RadioTaskSendPacket6      14



#define MAX_ERROR_PACKETS_PER_SEND_ALIVE_PERIOD   5
#define MAX_SEND_ALARM_ATTEMPTS                   10


//#define SEND_ALIVE_PERIOD 1800000000 / Clock_tickPeriod
#define SEND_ALIVE_PERIOD 5000000 / Clock_tickPeriod

extern UART_Handle uart;

void Log (const char *pcFormat, ...);


#endif /* APP_GLOBAL_H_ */
