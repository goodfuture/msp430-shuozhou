#ifndef _MODBUS_H
#define _MODBUS_H

#include "uart.h"
#include "prodata.h"

#define MODBUS_COM  COM5
//#define MODBUS_COM  COM4
//#define MODBUS_VARNUM 7
#define MODBUS_VARNUM 16

typedef enum {
	ERROR = 0,
	NORMAL,
} MB_STATUS;

typedef enum {
	INTEGER = 0,
	FLOAT,
} MB_TYPE;

typedef struct _MB_VAR_INFO {
	INT8U enable;
	MB_STATUS status;
	MB_TYPE type;
	INT8U devid;
	INT16U regaddr;
	INT16U ulv;
	INT16U llv;
	char regid[7];
}MB_VAR_INFO;

typedef struct _COM_PARAM {
	INT8U 		baud_index;
	MB_VAR_INFO mb_var[MODBUS_VARNUM];
} COM_PARAM;


extern COM_PARAM Com_Param;
//const char ComBaud_Str[4][6] = {"9600","19200","2400","4800"};
extern const char ComBaud_Str[4][6];
extern float Com_Val[MODBUS_VARNUM];
extern MAX_MIN_DATA Com_MaxMin[MODBUS_VARNUM];

extern INT8U MbOtCnt;
extern INT16U MbScanCnt;

void Modbus_Init(void);
void ProcessModbus(void);

#endif //end of _MODBUS_H

