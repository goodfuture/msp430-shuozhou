

#ifndef _MODBUSIO_H
#define _MODBUSIO_H

#include "uart.h"


#define MODBUSIO_COM  COM4
//#define MODBUSIO_COM  COM5
#define MODBUSIO_VARNUM 2

typedef struct _MBIO_PARAM
{
    INT8U baud_index;
    INT8U devid[MODBUSIO_VARNUM];
	INT8U funccode[MODBUSIO_VARNUM];
    INT16U regaddr[MODBUSIO_VARNUM];
	INT16U reglen[MODBUSIO_VARNUM];
} MBIO_PARAM;


extern INT8U MbIOOtCnt;
extern INT8U MbIODelayCnt;

void ModbusIO_Init(void);
void ProcessModbusIO(void);

#endif //end of _MODBUSIO_H

