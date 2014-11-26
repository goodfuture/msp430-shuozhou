#include <string.h>
#include "system.h"
#include "lcd.h"
#include "at24c64.h"
#include "ds1302.h"
#include "display.h"
#include "dio.h"
#include "control.h"
#include "flash.h"
#include "uart.h"
#include "sc16is752.h"
#include "pro212.h"
#include "modbusio.h"

//硬件操作宏定义
#define RUN_ON        P7OUT&=~BIT4
#define RUN_OFF       P7OUT|=BIT4

//程序版本号
const char VersionCode[6] = "VD05\0";
INT8U SoftWDT_Flag = 0;

INT8U HjtMsgSplitTimer;
INT8U LEDTimer;
INT8U RTCTimer;
INT16U RTCErrTimer;
INT8U System_TimeStr[16];
INT8U System_StartTimeStr[16];
T_Times Current_Tm;

void ProcRunLED(void)
{
	static char run = 0;

	if (LEDTimer >= S2TENMS(1)) {
		LEDTimer = 0;
		if (run == 0) {
			run = 1;
			RUN_ON;
		} else {
			run = 0;
			RUN_OFF;
		}
	}
}

void ProcRTC(void)
{
	static INT8U timestr_temp[16];

	//每950ms读一次RTC
	if (RTCTimer >= MS2TENMS(950)) {
		RTCTimer = 0;
		v_Get1302(System_TimeStr, &Current_Tm);
	}
	//每6s检测一下时钟是否停止，停止的话重新设置时钟
	if (RTCErrTimer > 3000) {
		RTCErrTimer = 0;
		if (strncmp((char *) timestr_temp, (char *) System_TimeStr,
				sizeof(timestr_temp)) != 0) {
			//时钟正常
			strncpy((char *) timestr_temp, (char *) System_TimeStr,
					sizeof(timestr_temp) - 1);
		} else {
			//时钟不正常
			v_Set1302(System_TimeStr);
		}
	}
}

void main(void)
{
	static char code[6];

	CPU_Init();
	FLASH_INIT()
	;
	GPRS_Init();
	LCD_Init();
	Clr_LCDRam();
	LCD_LIGHT_ON;
	Dis_Logo();

	Read_VersionCode(code);
	//读取配置数据
	if (memcmp(code, VersionCode, 6) == 0) {
		Read_SysParam(&Sys_Param);
		Read_ADParam(Ad_Param);
		Read_ComParam(&Com_Param);
	} else {
		Default_SysParam(&Sys_Param);
		Default_ADParam(Ad_Param);
		Default_ComParam(&Com_Param);
		Save_SysParam(&Sys_Param);
		Save_ADParam(Ad_Param);
		Save_ComParam(&Com_Param);
		Save_VersionCode((char *) VersionCode);
		//resetFlashIndexPage();
	}
	Read_ADCalibParam(&Ad_CalibParam);
	Fix_ADCalibParam(&Ad_CalibParam);
	ADParam_Calculate();

	v_Get1302(System_TimeStr, &Current_Tm);
	strcpy((char *) System_StartTimeStr, (char *) System_TimeStr);
	Dis_Welcome(1);

	IS752_Init(9600, COM2);
	IS752_Init(9600, COM3);
	IS752_Init(9600, COM4);
	IS752_Init(9600, COM5);
	ControlInit();
	Modbus_Init();
	//ModbusIO_Init();
	InitHjt212();
	ENABLEINT();

	while (1) {
		SoftWDT_Flag = 0;
		ProcRunLED();
		ProcRTC();
		ProcDIO();
		ProcADC();
		ProcessModbus();
		//ProcessModbusIO();
		ProcData();
		//ProcControl();
		ProcDisplay();
		ProcessHjt212();
	}
}
