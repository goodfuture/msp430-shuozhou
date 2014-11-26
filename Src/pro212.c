
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "system.h"
#include "control.h"
#include "uart.h"
#include "at24c64.h"
#include "ds1302.h"
#include "pro212.h"
#include "dio.h"

///////////////////////
#define GPRS_RST      P3OUT|=BIT4;Delay_10_uS();P3OUT&=~BIT4
#define GPRS_ONLINE   (!(P3IN&BIT3))
#define GPRS_POWERON  P7OUT|=BIT3
#define GPRS_POWEROFF P7OUT&=~BIT3

///////////////////////
#define HJT_STACK_NUM 1     //212协议栈的个数
#define HJT_UART 0
//#define HJT_ETH	 1

#define HJT_ERR_PW  -1
#define HJT_ERR_MN  -2
#define HJT_ERR_ST  -3
#define HJT_NOT_VALID -4

#define HJT_DATA_LEN 512

#define HJT_DEV_MSG_OFFSET 0
#define HJT_RTD_MSG_OFFSET 1
#define HJT_RTS_MSG_OFFSET 2
#define HJT_MIN_MSG_OFFSET 3
#define HJT_HOU_MSG_OFFSET 4
#define HJT_DAY_MSG_OFFSET 5

typedef struct _HJT212CMD {
	char qn[18];
	char flag[2];
	char pw[7];
	char mn[15];
	char cn[5];
	char* cp;
} HJT212CMD;

typedef struct _HJT212SIGNAL {
	char upflag_rt;
	char upflag_rs;
	char upflag_m;
	char upflag_h;
	char upflag_d;
} HJT212SIGNAL;

typedef struct _HjtCNTable {
	int (*func)(void*, int);
	char cn[6];
	int index;
} HjtCNTable;

//=====================================
//初始化GPRS
//=====================================
void GPRS_Init(void)
{
	GPRS_POWERON;
	GPRS_RST
	;
}

//=====================================
//212部分
//=====================================
unsigned int Hjt212RtdTimeCnt;	    //实时数据发送间隔计数器
unsigned char Hjt212TimeOffset;		//分钟数据的发送时间偏移
HJT212CMD Hjt212Cmd[HJT_STACK_NUM];	//212接收到的命令信息
HJT212SIGNAL Hjt212Signal[HJT_STACK_NUM] = { { 1, 1, 1, 1, 1 } };	//212上传数据标志
//=====================================
//212通讯中用到的全局变量
//=====================================
unsigned int HjtMsgSplitOk;
const int HJTUartChannel = GPRS_COM;					//dtu串口号
static unsigned char HJTState;					//212当前的处理状态
static unsigned int HJTRcvDataLen;				//212要接收的应答数据长度
static unsigned char HJTRcvDataBuf[HJT_DATA_LEN];	//212接收的数据存放缓冲区
static unsigned int HJTRcved;					//212已接收的数据长度

//函数声明
static int report_systemtime(void* cp, int index);
static int set_systemtime(void* cp, int index);
static int report_rtdInterval(void* cp, int index);
static int set_rtdInterval(void* cp, int index);
static int set_password(void* cp, int index);
static int report_sim(void* cp, int index);
static int set_sim(void* cp, int index);

static int report_worktype(void* cp, int index);
static int set_worktype(void* cp, int index);
static int report_gasruno2(void* cp, int index);
static int set_gasruno2(void* cp, int index);
static int report_gasstopo2(void* cp, int index);
static int set_gasstopo2(void* cp, int index);

static int report_gasswitchlong(void* cp, int index);
static int set_gasswitchlong(void* cp, int index);
static int report_vlswitchlong(void* cp, int index);
static int set_vlswitchlong(void* cp, int index);
static int report_vlswitchlast(void* cp, int index);
static int set_vlswitchlast(void* cp, int index);

static int start_reportRealData(void* cp, int index);
static int stop_reportRealData(void* cp, int index);
static int start_reportRealState(void* cp, int index);
static int stop_reportRealState(void* cp, int index);

static int set_dooutput(void *cp, int index);

static int response_HistoryData(void* cp, int index);
static void responseNotice(int index);
static void reportHistoryData(char *p_begintime, char *p_endtime, int cmd,
		int index);
static void sendHjtPkt(char *buf, int index);
static void Hjt212UploadMsg(INT8U *msgflag);

//命令动作表
const HjtCNTable CNTable[] = { { report_systemtime, "1011\0", 0 }, {
		set_systemtime, "1012\0", 1 }, { report_rtdInterval, "1061\0", 2 }, {
		set_rtdInterval, "1062\0", 3 }, { set_password, "1072\0", 4 }, {
		report_sim, "1091\0", 5 }, { set_sim, "1092\0", 6 }, { report_gasruno2,
		"1101\0", 7 }, { set_gasruno2, "1102\0", 8 }, { report_gasstopo2,
		"1111\0", 9 }, { set_gasstopo2, "1112\0", 10 }, { report_gasswitchlong,
		"1121\0", 11 }, { set_gasswitchlong, "1122\0", 12 }, {
		report_vlswitchlong, "1131\0", 13 }, { set_vlswitchlong, "1132\0", 14 },
		{ report_vlswitchlast, "1141\0", 15 },
		{ set_vlswitchlast, "1142\0", 16 }, { report_worktype, "1161\0", 17 }, {
				set_worktype, "1162\0", 18 }, { start_reportRealData, "2011\0",
				19 }, { stop_reportRealData, "2012\0", 20 }, {
				start_reportRealState, "2021\0", 21 }, { stop_reportRealState,
				"2022\0", 22 }, { response_HistoryData, "2031\0", 23 }, {
				response_HistoryData, "2051\0", 24 }, { response_HistoryData,
				"2061\0", 25 }, { set_dooutput, "3017\0", 26 },
		{ 0, "\0", 0xff } };

//=====================================
//保存212配置
//=====================================
void paramWrite(void)
{
	Save_SysParam(&Sys_Param);
}
//=====================================
//212 crc计算函数
//=====================================
unsigned short hjt212Crc(char * pData, int nLength)
{
	unsigned short wCrc = 0xffff;
	unsigned char temp;
	unsigned char *pd = (unsigned char *) pData;
	int i, j;

	for (i = 0; i < nLength; i++) {
		temp = (unsigned char) (wCrc >> 8);
		temp ^= (unsigned short) pd[i];
		wCrc = temp;
		for (j = 0; j < 8; j++) {
			if (wCrc & 1) {
				wCrc >>= 1;
				wCrc &= 0x7fff;
				wCrc ^= 0xA001;
			} else {
				wCrc >>= 1;
				wCrc &= 0x7fff;
			}
		}
	}

	return wCrc;
}

//-----------------------------------------
//构建212协议帧头
//-----------------------------------------
static void hjt212Head(const char* cn, char* buf)
{
	int len = 0;
	char st[3];  //used for 9011 9012 9013

	if ((strncmp(cn, "9011", 4) == 0) || (strncmp(cn, "9012", 4) == 0)
			|| (strncmp(cn, "9013", 4) == 0)) {
		st[0] = '9';
		st[1] = '1';
		st[2] = 0;
	} else {
		strcpy(st, (char *) Sys_Param.st);
	}

	sprintf(buf, "##%.4dST=%s;CN=%s;PW=%s;MN=%s;", len, st, cn, Sys_Param.pw,
			Sys_Param.mn);
}

//-----------------------------------------
//修改212数据长度值
//-----------------------------------------
static int hjt212Length(char* buf, int len)
{
	char temp[5];
	int i;

	if (len > 1024)
		return -1;
	sprintf(temp, "%.4d", len);
	for (i = 0; i < 4; i++)
		*(buf + 2 + i) = temp[i];

	return 0;
}

//1011
void getSysTimeStr(char *p)
{
	strcpy(p, (char const *) System_TimeStr);
}
static int report_systemtime(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	char systime[20];
	cp = cp;

	//##....MN=..;
	hjt212Head("1011", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";SystemTime=");
	getSysTimeStr(systime);
	strcat(buf, systime);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}

//1012
void setSysTimeFromStr(char *p)
{
	v_Set1302((INT8U *) p);
}
static int set_systemtime(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;
	char timetmp[20] = "\0";

	index = index;
	q = strstr(p, "SystemTime=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;

	if (q && r) {
		memset(timetmp, 0, sizeof(timetmp));
		strncpy(timetmp, q + strlen("SystemTime="),
				(int) (r - q - strlen("SystemTime=")));
	} else {
		return 2;
	}

	setSysTimeFromStr(timetmp);

	return 1;
}

//1061
static int report_rtdInterval(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	//##....MN=..;
	hjt212Head("1061", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";RtdInterval=");
	strcat(buf, (char *) Sys_Param.rtd);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1062
static int set_rtdInterval(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;

	index = index;
	q = strstr(p, "RtdInterval=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset((char *) Sys_Param.rtd, 0, sizeof(Sys_Param.rtd));
		strncpy((char *) Sys_Param.rtd, q + 12, (int) (r - q - 12));
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1072
static int set_password(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;

	index = index;
	q = strstr(p, "PW=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset((char *) Sys_Param.pw, 0, sizeof(Sys_Param.pw));
		strncpy((char *) Sys_Param.pw, q + 3, (int) (r - q - 3));
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1091
static int report_sim(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	//##....MN=..;
	hjt212Head("1091", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";SIM=");
	strcat(buf, (char *) Sys_Param.sim);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1092
static int set_sim(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;

	index = index;
	q = strstr(p, "SIM=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset((char *) Sys_Param.sim, 0, sizeof(Sys_Param.sim));
		strncpy((char *) Sys_Param.sim, q + 4, (int) (r - q - 4));
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1101
static int report_gasruno2(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	//##....MN=..;
	hjt212Head("1101", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";GasRunO2=");
	sprintf(temp, "%.4f", Sys_Param.dolow);
	strcat(buf, temp);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1102
static int set_gasruno2(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;
	char temp[100];

	index = index;
	q = strstr(p, "GasRunO2=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset(temp, 0, sizeof(temp));
		strncpy(temp, q + strlen("GasRunO2="),
				(int) (r - q - strlen("GasRunO2=")));
		Sys_Param.dolow = atof(temp);
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1111
static int report_gasstopo2(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	//##....MN=..;
	hjt212Head("1111", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";GasStopO2=");
	sprintf(temp, "%.4f", Sys_Param.dohigh);
	strcat(buf, temp);
	strcat(buf, "&&");

	len = strlen(buf) - 6;   //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1112
static int set_gasstopo2(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;
	char temp[100];

	index = index;
	q = strstr(p, "GasStopO2=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset(temp, 0, sizeof(temp));
		strncpy(temp, q + strlen("GasStopO2="),
				(int) (r - q - strlen("GasStopO2=")));
		Sys_Param.dohigh = atof(temp);
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1121
static int report_gasswitchlong(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	//##....MN=..;
	hjt212Head("1121", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";GasSwitchLong=");
	sprintf(temp, "%d", Sys_Param.engine_switchlong);
	strcat(buf, temp);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1122
static int set_gasswitchlong(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;
	char temp[100];

	index = index;
	q = strstr(p, "GasSwitchLong=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset(temp, 0, sizeof(temp));
		strncpy(temp, q + strlen("GasSwitchLong="),
				(int) (r - q - strlen("GasSwitchLong=")));
		Sys_Param.engine_switchlong = atoi(temp);
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1131
static int report_vlswitchlong(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	//##....MN=..;
	hjt212Head("1131", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";VLSwitchLong=");
	sprintf(temp, "%d", Sys_Param.value_switchlong);
	strcat(buf, temp);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1132
static int set_vlswitchlong(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;
	char temp[100];

	index = index;
	q = strstr(p, "VLSwitchLong=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset(temp, 0, sizeof(temp));
		strncpy(temp, q + strlen("VLSwitchLong="),
				(int) (r - q - strlen("VLSwitchLong=")));
		Sys_Param.value_switchlong = atoi(temp);
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1141
static int report_vlswitchlast(void* cp, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	//##....MN=..;
	hjt212Head("1141", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";VLSwitchLast=");
	sprintf(temp, "%d", Sys_Param.value_switchlast);
	strcat(buf, temp);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1142
static int set_vlswitchlast(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;
	char temp[100];

	index = index;
	q = strstr(p, "VLSwitchLast=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		memset(temp, 0, sizeof(temp));
		strncpy(temp, q + strlen("VLSwitchLast="),
				(int) (r - q - strlen("VLSwitchLast=")));
		Sys_Param.value_switchlast = atoi(temp);
		paramWrite();
	} else {
		return 2;
	}

	return 1;
}

//1161
static int report_worktype(void* cp, int index)
{
	char buf[1024];
	char c, temp[100];
	int len;
	unsigned short crc;
	cp = cp;

	if (Get_WorkType() == WORKTYPE_AUTO)
		c = 'A';
	else if (Get_WorkType() == WORKTYPE_MANUAL)
		c = 'M';
	else
		c = 'R';

	//##....MN=..;
	hjt212Head("1161", buf);
	strcat(buf, "CP=&&QN=");
	strcat(buf, Hjt212Cmd[index].qn);
	strcat(buf, ";WorkType=");
	sprintf(temp, "%c", c);
	strcat(buf, temp);
	strcat(buf, "&&");

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}
//1162
static int set_worktype(void* cp, int index)
{
	char* p = (char*) cp;
	char* q, *r;

	index = index;
	q = strstr(p, "WorkType=");
	if (q)
		r = strstr(q, "&&");
	else
		return 2;
	if (q && r) {
		if (*(q + 9) == 'A') {
			Sys_Param.worktype = WORKTYPE_AUTO;
			paramWrite();
		} else if ((*(q + 9) == 'M') || (*(q + 9) == 'R')) {
			Sys_Param.worktype = WORKTYPE_REMOTE;
			paramWrite();
		} else {
			return 2;
		}
	} else {
		return 2;
	}

	return 1;
}

//2011
static int start_reportRealData(void* cp, int index)
{
	cp = cp;
	Hjt212Signal[index].upflag_rt = 1;
	return 1;
}
//2012
static int stop_reportRealData(void* cp, int index)
{
	cp = cp;
	Hjt212Signal[index].upflag_rt = 0;
	responseNotice(index);
	return 1;
}

//2021
static int start_reportRealState(void* cp, int index)
{
	cp = cp;
	Hjt212Signal[index].upflag_rs = 1;
	return 1;
}
//2022
static int stop_reportRealState(void* cp, int index)
{
	cp = cp;
	Hjt212Signal[index].upflag_rs = 0;
	responseNotice(index);
	return 1;
}

//2051, 2061, 2031, 2081
static int response_HistoryData(void* cp, int index)
{
	char begintime[20], endtime[20];
	char* p = (char*) cp;
	char* q;
	int cmd;

	q = strstr(p, "BeginTime=");
	if (NULL == q)
		return 2;

	strncpy(begintime, q + 10, 14);
	begintime[14] = 0;

	q = strstr(p, "EndTime=");
	if (NULL == q)
		return 2;

	strncpy(endtime, q + 8, 14);
	endtime[14] = 0;

	cmd = atoi(Hjt212Cmd[index].cn);

	reportHistoryData(begintime, endtime, cmd, index);

	return 1;
}

//9011
static void responseQnRtn(int rt, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;

	hjt212Head("9011", buf);
	sprintf(temp, "Flag=0;CP=&&QN=%s;QnRtn=%d&&", Hjt212Cmd[index].qn, rt);
	strcat(buf, temp);

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);
}

//9012
static void responseExeRtn(int ert, int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;

	hjt212Head("9012", buf);
	sprintf(temp, "CP=&&QN=%s;ExeRtn=%d&&", Hjt212Cmd[index].qn, ert);
	strcat(buf, temp);

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);
}

//9013
static void responseNotice(int index)
{
	char buf[1024];
	char temp[100];
	int len;
	unsigned short crc;

	hjt212Head("9013", buf);
	sprintf(temp, "CP=&&QN=%s&&", Hjt212Cmd[index].qn);
	strcat(buf, temp);

	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);
}

//2011上报实时数据
int reportRealData(int index)
{
	char buf[1024], temp[100], systime[20];
	int i, len;
	unsigned short crc;

	//##....MN=..;
	memset(buf, 0, sizeof(buf));
	hjt212Head("2011", buf);
	strcat(buf, "CP=&&DataTime=");
	getSysTimeStr(systime);
	strcat(buf, systime);

	///////////////////
	for (i = 0; i < 8; ++i) {
		//if((Ad_Param[i].id[0] < '0') || (Ad_Param[i].id[1] < '0') ||
		//(Ad_Param[i].id[2] < '0'))
		if (Ad_Param[i].enable == 0)
			continue;
		//if(strcmp((char *)Ad_Param[i].id, "000") == 0)
		//continue;

		sprintf(temp, ";%s-Rtd=%f", Ad_Param[i].id, Ad_RealData[i]);
		strcat(buf, temp);
	}
	for (i = 0; i < MODBUS_VARNUM; ++i) {
		if (Com_Param.mb_var[i].enable == 0) {
			continue;
		}

		sprintf(temp, ";%s-Rtd=%f", Com_Param.mb_var[i].regid, Com_Val[i]);
		strcat(buf, temp);
	}
	/////////////////////

	strcat(buf, "&&");
	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	/*if(HJT_UART == index)
	 saveRtdData(&Current_Tm,buf);*/

	return 1;
}

//2021上报实时设备状态
int reportRealState(int index)
{
	char buf[1024], temp[100], systime[20];
	char sc;
	int i, len;
	unsigned short crc;
	INT8U stat;

	//##....MN=..;
	hjt212Head("2021", buf);
	strcat(buf, "CP=&&DataTime=");
	getSysTimeStr(systime);
	strcat(buf, systime);

	///////////////////
	stat = Get_WorkType();
	if (stat == WORKTYPE_AUTO)
		sc = 'A';
	else if (stat == WORKTYPE_MANUAL)
		sc = 'M';
	else
		sc = 'R';
	sprintf(temp, ";WorkType=%c", sc);
	strcat(buf, temp);

	for (i = 0; i < 2; ++i) {
		stat = Get_PumpStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";P%dS=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_EngineStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";F%dS=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_ValueStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'O';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'C';
		sprintf(temp, ";EV%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_BackflowStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";BF%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_ReuseStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";RU%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 3; ++i) {
		stat = Get_LevelStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == LEVEL_HIGH)
			sc = 'H';
		else if (stat == LEVEL_LOW)
			sc = 'L';
		else
			sc = 'M';
		sprintf(temp, ";LV%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	if (PWR_Stat == DEVICE_RUN)
		sc = 'R';
	else if (PWR_Stat == DEVICE_FAULT)
		sc = 'F';
	else
		sc = 'T';
	sprintf(temp, ";PWR=%c", sc);
	strcat(buf, temp);
	/////////////////////

	strcat(buf, "&&");
	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}

//2051 2061 2031
//上报分钟2051 小时2061 日2031 数据
//cmd为212协议的命令号
void reportData_M_H_D(int cmd, int index)
{
	char buf[1024], temp[100], systime[20], name[4];
	int i, len;
	unsigned short crc;

	switch (cmd) {
	case 2051:
		hjt212Head("2051", buf);
		getSysTimeStr(systime);
		strncpy(temp, systime, 12);
		temp[12] = '0';
		temp[13] = '0';
		temp[14] = 0;
		break;
	case 2061:
		hjt212Head("2061", buf);
		getSysTimeStr(systime);
		strncpy(temp, systime, 10);
		for (i = 0; i < 4; i++)
			temp[10 + i] = '0';
		temp[14] = 0;
		break;
	case 2031:
		hjt212Head("2031", buf);
		getSysTimeStr(systime);
		strncpy(temp, systime, 8);
		for (i = 0; i < 6; i++)
			temp[8 + i] = '0';
		temp[14] = 0;
		break;
	default:
		return;
	}
	strcat(buf, "CP=&&DataTime=");
	strcat(buf, temp);

	///////////////////
	for (i = 0; i < 8; ++i) {
		if (Ad_Param[i].enable == 0) {
			continue;
		}
#if 0
		if((Ad_Param[i].id[0] < '0') || (Ad_Param[i].id[1] < '0') ||
				(Ad_Param[i].id[2] < '0'))
		continue;
		if(strcmp((char *)Ad_Param[i].id, "000") == 0)
		continue;
#endif

		//历史min，avg，max数据
		sprintf(name, "%s", Ad_Param[i].id);
		switch (cmd) {
		case 2051:
			sprintf(temp, ";%s-Min=%f,%s-Avg=%f,%s-Max=%f,%s-Cou=%f", name,
					Ad_MaxMin[i].mnt_min, name, Ad_MaxMin[i].mnt_avg, name,
					Ad_MaxMin[i].mnt_max, name, Ad_MaxMin[i].mnt_cou);
			break;
		case 2061:
			sprintf(temp, ";%s-Min=%f,%s-Avg=%f,%s-Max=%f,%s-Cou=%f", name,
					Ad_MaxMin[i].hour_min, name, Ad_MaxMin[i].hour_avg, name,
					Ad_MaxMin[i].hour_max, name, Ad_MaxMin[i].hour_cou);
			break;
		case 2031:
			sprintf(temp, ";%s-Min=%f,%s-Avg=%f,%s-Max=%f,%s-Cou=%f", name,
					Ad_MaxMin[i].day_min, name, Ad_MaxMin[i].day_avg, name,
					Ad_MaxMin[i].day_max, name, Ad_MaxMin[i].day_cou);
			break;
		default:
			return;
		}
		strcat(buf, temp);
	}

	for (i = 0; i < MODBUS_VARNUM; ++i) {
		if (Com_Param.mb_var[i].enable == 0) {
			continue;
		}
#if 0
		if(strncmp((char *)Com_Param.regid[i], "000000", 6) == 0)
		continue;
#endif

		//历史min，avg，max数据
		sprintf(name, "%s", Com_Param.mb_var[i].regid);
		switch (cmd) {
		case 2051:
			sprintf(temp, ";%s-Min=%f,%s-Avg=%f,%s-Max=%f,%s-Cou=%f", name,
					Com_MaxMin[i].mnt_min, name, Com_MaxMin[i].mnt_avg, name,
					Com_MaxMin[i].mnt_max, name, Com_MaxMin[i].mnt_cou);
			break;
		case 2061:
			sprintf(temp, ";%s-Min=%f,%s-Avg=%f,%s-Max=%f,%s-Cou=%f", name,
					Com_MaxMin[i].hour_min, name, Com_MaxMin[i].hour_avg, name,
					Com_MaxMin[i].hour_max, name, Com_MaxMin[i].hour_cou);
			break;
		case 2031:
			sprintf(temp, ";%s-Min=%f,%s-Avg=%f,%s-Max=%f,%s-Cou=%f", name,
					Com_MaxMin[i].day_min, name, Com_MaxMin[i].day_avg, name,
					Com_MaxMin[i].day_max, name, Com_MaxMin[i].day_cou);
			break;
		default:
			return;
		}
		strcat(buf, temp);
	}

	ClearStatisData(cmd);
	/////////////////////

	strcat(buf, "&&");
	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	/*if(HJT_UART == index)
	 {
	 switch (cmd)
	 {
	 case 2051:
	 saveMtdData(&Current_Tm,buf);
	 break;
	 case 2061:
	 saveHtdData(&Current_Tm,buf);
	 break;
	 case 2031:
	 saveDtdData(&Current_Tm,buf);
	 break;
	 default:
	 break;
	 }
	 }*/
}

//上报分钟2051 小时2061 日2031 的历史数据，需要从存储文件中读
//type0-3代表分钟数据，小时数据，日数据，实时数据
void reportHistoryData(char *p_begintime, char *p_endtime, int cmd, int index)
{
	/*T_Times bt, et;
	 char buf[1030];

	 if(str2TM(p_begintime,&bt))
	 return;
	 if(str2TM(p_endtime,&et))
	 return;

	 switch (cmd)
	 {
	 case 2051:
	 checkoutMtdData(&bt,&et,buf);
	 break;
	 case 2061:
	 checkoutHtdData(&bt,&et,buf);
	 break;
	 case 2031:
	 checkoutDtdData(&bt,&et,buf);
	 break;
	 default:
	 break;
	 }*/
}

//3015上传逢变则报
static int report_DevstateChange(int index)
{
	char buf[1024], temp[100], temp2[100], systime[20];
	char sc;
	int i, len;
	unsigned short crc;
	INT8U stat;

	//##....MN=..;
	memset(buf, 0, sizeof(buf));
	getSysTimeStr(systime);
	hjt212Head("3015", buf);
	strcpy(temp2, &buf[6]);
	sprintf(temp, "QN=%s000;", systime);
	strcpy(&buf[6], temp);
	strcat(buf, temp2);
	strcat(buf, "CP=&&DataTime=");
	strcat(buf, systime);

	///////////////////
	stat = Get_WorkType();
	if (stat == WORKTYPE_AUTO)
		sc = 'A';
	else if (stat == WORKTYPE_MANUAL)
		sc = 'M';
	else
		sc = 'R';
	sprintf(temp, ";WorkType=%c", sc);
	strcat(buf, temp);

	for (i = 0; i < 2; ++i) {
		stat = Get_PumpStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";P%dS=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_EngineStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";F%dS=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_ValueStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'O';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'C';
		sprintf(temp, ";EV%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_BackflowStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";BF%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 2; ++i) {
		stat = Get_ReuseStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == DEVICE_RUN)
			sc = 'R';
		else if (stat == DEVICE_FAULT)
			sc = 'F';
		else
			sc = 'T';
		sprintf(temp, ";RU%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	for (i = 0; i < 3; ++i) {
		stat = Get_LevelStat(i);

		if (stat == DEVICE_DISABLE)
			continue;

		if (stat == LEVEL_HIGH)
			sc = 'H';
		else if (stat == LEVEL_LOW)
			sc = 'L';
		else
			sc = 'M';
		sprintf(temp, ";LV%d=%c", i + 1, sc);
		strcat(buf, temp);
	}

	if (PWR_Stat == DEVICE_RUN)
		sc = 'R';
	else if (PWR_Stat == DEVICE_FAULT)
		sc = 'F';
	else
		sc = 'T';
	sprintf(temp, ";PWR=%c", sc);
	strcat(buf, temp);
	/////////////////////

	strcat(buf, "&&");
	len = strlen(buf) - 6;  //except "##0000"
	hjt212Length(buf, len);  //add length of the bag
	crc = hjt212Crc(buf + 6, len);  //except "##0000"
	sprintf(temp, "%.4X\r\n", crc);
	strcat(buf, temp);

	sendHjtPkt(buf, index);

	return 1;
}

//3017设置DO状态
static int set_dooutput(void *cp, int index)
{
	char* p = (char*) cp;
	char* q;
	char i, sc;

	index = index;

	/////////////////////
	for (i = 0; i < 2; ++i) {
		if (i == 0)
			q = strstr(p, "P1S=");
		else if (i == 1)
			q = strstr(p, "P2S=");

		if (q) {
			sc = *(q + 4);
			if (sc == 'S')
				Start_Pump(i, 0);
			else if (sc == 'T')
				Stop_Pump(i);
		}
	}

	for (i = 0; i < 2; ++i) {
		if (i == 0)
			q = strstr(p, "F1S=");
		else if (i == 1)
			q = strstr(p, "F2S=");

		if (q) {
			sc = *(q + 4);
			if (sc == 'S')
				Start_Engine(i, 0);
			else if (sc == 'T')
				Stop_Engine(i);
		}
	}

	for (i = 0; i < 2; ++i) {
		if (i == 0)
			q = strstr(p, "EV1=");
		else if (i == 1)
			q = strstr(p, "EV2=");

		if (q) {
			sc = *(q + 4);
			if (sc == 'O')
				Start_Value(i, 0);
			else if (sc == 'C')
				Stop_Value(i);
		}
	}

	for (i = 0; i < 2; ++i) {
		if (i == 0)
			q = strstr(p, "BF1=");
		else if (i == 1)
			q = strstr(p, "BF2=");

		if (q) {
			sc = *(q + 4);
			if (sc == 'S')
				Start_Backflow(i, 0);
			else if (sc == 'T')
				Stop_Backflow(i);
		}
	}

	for (i = 0; i < 2; ++i) {
		if (i == 0)
			q = strstr(p, "RU1=");
		else if (i == 1)
			q = strstr(p, "RU2=");

		if (q) {
			sc = *(q + 4);
			if (sc == 'S')
				Start_Reuse(i, 0);
			else if (sc == 'T')
				Stop_Reuse(i);
		}
	}
	/////////////////////

	return 1;
}

//=====================================
//212报文发送函数
//=====================================
void sendHjtPkt(char *buf, int index)
{
	char *p;
	int cm;

	p = memchr(Sys_Param.sim, '\0', 11);
	cm = memcmp(Sys_Param.sim, "00000000000", 11);
	if ((NULL == p) && (cm != 0)) {
		Uart_SendData((unsigned char *) Sys_Param.sim, 11, HJTUartChannel);
	}

	Uart_SendData((unsigned char *) buf, strlen(buf), HJTUartChannel);
}

//-----------------------------------------
//212数据分析函数
//-----------------------------------------
static int hjt212ParseMessage(char* msg, int* err, int index)
{
	char* p;

	p = strstr(msg, "MN=");
	if (p) {
		strncpy(Hjt212Cmd[index].mn, p + 3, 14);
		if (strncmp(Hjt212Cmd[index].mn, (char *) Sys_Param.mn, 14) != 0) {
			*err = HJT_ERR_MN;
			return -2;
		}
	} else {
		*err = HJT_NOT_VALID;
		return -1;
	}

	p = strstr(msg, "PW=");
	if (p) {
		strncpy(Hjt212Cmd[index].pw, p + 3, 6);
		if (strncmp(Hjt212Cmd[index].pw, (char *) Sys_Param.pw, 6) != 0) {
			*err = HJT_ERR_PW;
			return -2;
		}
	} else {
		*err = HJT_NOT_VALID;
		return -1;
	}

	p = strstr(msg, "QN=");
	if (p) {
		strncpy(Hjt212Cmd[index].qn, p + 3, 17);
	} else {
		*err = HJT_NOT_VALID;
		return -1;
	}

	p = strstr(msg, "CN=");
	if (p) {
		strncpy(Hjt212Cmd[index].cn, p + 3, 4);
	} else {
		*err = HJT_NOT_VALID;
		return -1;
	}

	p = strstr(msg, "Flag=");
	if (p) {
		strncpy(Hjt212Cmd[index].flag, p + 5, 1);
	} else {
		if (strncmp(Hjt212Cmd[index].cn, "2012", 4) == 0) {
			strncpy(Hjt212Cmd[index].flag, "0", 1);
		} else {
			*err = HJT_NOT_VALID;
			return -1;
		}
	}

	p = strstr(msg, "CP=&&");
	if (p) {
		Hjt212Cmd[index].cp = p;
	} else {
		*err = HJT_NOT_VALID;
		return -1;
	}

	*err = 0;
	return 0; //ok
}

//-----------------------------------------
//212报文处理函数
//-----------------------------------------
void hjt212Access(char* msg, int index)
{
	const HjtCNTable* pcntable;
	int err;

	if (hjt212ParseMessage(msg, &err, index)) {
		switch (err) {
		case HJT_ERR_PW:
			responseQnRtn(3, index); //passwd error
			return;
		case HJT_ERR_MN:
			return;
		case HJT_NOT_VALID:
			memset(msg, 0, HJT_DATA_LEN);
			return;
		default:
			break;
		}
	}

	switch (Hjt212Cmd[index].flag[0]) {
	case '0':
	case '2':
		break;
	case '1':
	case '3':
		responseQnRtn(1, index); //normal
		break;
	default:
		break;
	}

	for (pcntable = CNTable; pcntable->index != 0xff; pcntable++) {
		if (strncmp(Hjt212Cmd[index].cn, pcntable->cn, 4) == 0) {
			int rtn;
			rtn = pcntable->func(Hjt212Cmd[index].cp, index);
			if ((strncmp(Hjt212Cmd[index].cn, "2011", 4) != 0)
					&& (strncmp(Hjt212Cmd[index].cn, "2012", 4) != 0))
				responseExeRtn(rtn, index);
			return;
		}
	}

	responseExeRtn(2, index);  //no such cn
}

//=====================================
//212命令接收函数, 返回0表示继续接收，1表示接收完成
//=====================================
int rcvHjtCmd(void)
{
	int ret;
	unsigned char c;
	char temp[5];
	unsigned short crc1, crc2;

	while (1) {
		ret = Uart_RcvData(&c, 1, HJTUartChannel);
		if (ret < 1) {
			return 0;
		}

		if ('#' == c) {
			if (HJTRcved != 1)	//包头
					{
				HJTRcvDataBuf[0] = c;
				HJTRcved = 1;
			} else				//第二个'#'
			{
				HJTRcvDataBuf[1] = c;
				HJTRcved = 2;
			}
		} else {
			if (HJTRcved < 2) {
				HJTRcved = 0;
				continue;
			}

			HJTRcvDataBuf[HJTRcved] = c;
			HJTRcved++;
			if (6 == HJTRcved) {
				memcpy(temp, &HJTRcvDataBuf[2], 4);
				temp[4] = 0;
				HJTRcvDataLen = atoi(temp) + 12;
			} else if ((HJTRcved > 6) && (HJTRcved >= HJTRcvDataLen)) {
				if ((HJTRcvDataBuf[HJTRcvDataLen - 2] != '\r')
						|| (HJTRcvDataBuf[HJTRcvDataLen - 1] != '\n')) {
					HJTRcved = 0;
					//HJTRcvDataLen = 0;
					return 0;
				}

				memcpy(temp, &HJTRcvDataBuf[HJTRcvDataLen - 6], 4);
				temp[4] = 0;
				sscanf(temp, "%x", &crc1);
				crc2 = hjt212Crc((char *) (&HJTRcvDataBuf[6]),
						HJTRcvDataLen - 12);
				HJTRcvDataBuf[HJTRcvDataLen] = 0;

				if (crc1 != crc2) {
					HJTRcved = 0;
					//HJTRcvDataLen = 0;
					return 0;
				}

				HJTRcved = 0;
				//HJTRcvDataLen = 0;
				return 1;
			}
		}	//end of if c=='#'
	}	//end of while
}

//=====================================
//212命令处理函数
//=====================================
void Process212Cmd(void)
{
	int flag, ret;

	flag = 0;
	while (1) {
		switch (HJTState) {
		case 0:		//接收命令
			ret = rcvHjtCmd();
			if (0 == ret) {
				flag = 1;
			} else {
				HJTState = 1;
			}
			break;
		case 1:		//分析命令
			hjt212Access((char *) HJTRcvDataBuf, HJT_UART);

			HJTState = 0;
			flag = 1;
			break;
		default:
			flag = 1;
			break;
		}

		if (1 == flag)
			break;
	}
}

//=====================================
//212发包函数
//=====================================
void Hjt212UploadMsg(INT8U *msgflag)
{
	if (HjtMsgSplitTimer > 30) {
		HjtMsgSplitOk = 1;
	}

	if (!HjtMsgSplitOk || !(*msgflag))
		return;

	if (*msgflag & (1 << HJT_DEV_MSG_OFFSET)) //设备状态
			{
		*msgflag &= ~(1 << HJT_DEV_MSG_OFFSET);
		//report_DevstateChange(HJT_UART);
	} else if (*msgflag & (1 << HJT_RTD_MSG_OFFSET)) 	//实时数据
			{
		*msgflag &= ~(1 << HJT_RTD_MSG_OFFSET);
		reportRealData(HJT_UART);
	} else if (*msgflag & (1 << HJT_RTS_MSG_OFFSET))  	//实时状态
			{
		*msgflag &= ~(1 << HJT_RTS_MSG_OFFSET);
		//reportRealState(HJT_UART);
	} else if (*msgflag & (1 << HJT_MIN_MSG_OFFSET))	//分钟报文
			{
		*msgflag &= ~(1 << HJT_MIN_MSG_OFFSET);
		reportData_M_H_D(2051, HJT_UART);
	} else if (*msgflag & (1 << HJT_HOU_MSG_OFFSET)) 	//小时报文
			{
		*msgflag &= ~(1 << HJT_HOU_MSG_OFFSET);
		reportData_M_H_D(2061, HJT_UART);
	} else if (*msgflag & (1 << HJT_DAY_MSG_OFFSET)) 	//日报文
			{
		*msgflag &= ~(1 << HJT_DAY_MSG_OFFSET);
		reportData_M_H_D(2031, HJT_UART);
	}

	HjtMsgSplitOk = 0;
	HjtMsgSplitTimer = 0;
}

//=====================================
//212上传处理函数
//=====================================
void Process212Upload(void)
{
	static int LastSecond;
	int interval;
	int secnow, minnow, hournow;
	static INT32U Last_DIVal;
	static INT8U Last_PWRStat;
	static INT8U FirstSend = 0;
	static INT8U MsgFlag = 0;		//212报文待上传标识，按位区分不同报文

	Hjt212UploadMsg(&MsgFlag);

	//逢变则报
	if (FirstSend == 0) {
		FirstSend = 1;
		MsgFlag |= 1 << HJT_DEV_MSG_OFFSET;
	} else if ((DI_Val != Last_DIVal) || (PWR_Stat != Last_PWRStat)) {
		MsgFlag |= 1 << HJT_DEV_MSG_OFFSET;
	}
	Last_DIVal = DI_Val;
	Last_PWRStat = PWR_Stat;

	//判断时间
	secnow = Current_Tm.second;
	minnow = Current_Tm.minute;
	hournow = Current_Tm.hour;
	if (LastSecond == secnow)
		return;

	LastSecond = secnow;

	//当实时时间计数间隔到则发送实时数据
	interval = atoi((char *) Sys_Param.rtd);
	if (++Hjt212RtdTimeCnt >= interval) {
		Hjt212RtdTimeCnt = 0;
		if (Hjt212Signal[HJT_UART].upflag_rt)
			MsgFlag |= 1 << HJT_RTD_MSG_OFFSET;
		if (Hjt212Signal[HJT_UART].upflag_rs)
			MsgFlag |= 1 << HJT_RTS_MSG_OFFSET;
	}

	//每3分钟检测GPRS是否在线
	if ((secnow == 0) && ((minnow % 3) == 0)) {
		if (!(GPRS_ONLINE))		//GPRS检测
			GPRS_RST
		;
	}

	//分钟数据在十分钟的第Hjt212TimeOffset秒发送
	if (((minnow % 10) == 0) && (secnow == Hjt212TimeOffset))	//minute data
			{
		if (Hjt212Signal[HJT_UART].upflag_m)
			MsgFlag |= 1 << HJT_MIN_MSG_OFFSET;
	}

	//小时数据在一个小时的第Hjt212TimeOffset+2秒发送
	if ((minnow == 0) && (secnow == Hjt212TimeOffset + 10))	//hour data
			{
		if (Hjt212Signal[HJT_UART].upflag_h)
			MsgFlag |= 1 << HJT_HOU_MSG_OFFSET;
	}
	//日数据在一天的第Hjt212TimeOffset+4秒发送
	if ((hournow == 0) && (minnow == 0) && (secnow == Hjt212TimeOffset + 20))//day data
			{
		if (Hjt212Signal[HJT_UART].upflag_d)
			MsgFlag |= 1 << HJT_DAY_MSG_OFFSET;
	}
	//-----------在0:0:50时，重启GPRS，防止GPRS假死（0:0:51上电）---------------
	if ((hournow == 0) && (minnow == 0) && (secnow == 50)) {
		GPRS_POWEROFF;
	}
	if ((hournow == 0) && (minnow == 0) && (secnow == 51)) {
		GPRS_POWERON;
	}
}

//=====================================
//212协议处理函数
//=====================================
void ProcessHjt212(void)
{
	//Process212Cmd();
	Process212Upload();
}

//=====================================
//初始化212
//=====================================
void InitHjt212(void)
{
	//将当前时间秒数的个位值作为212时间偏移	
	int secondnow = Current_Tm.second;
	Hjt212TimeOffset = (secondnow % 10);
	Hjt212RtdTimeCnt = 0;
}

