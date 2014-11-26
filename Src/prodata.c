#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "system.h"
#include "prodata.h"
#include "ad.h"
#include "modbus.h"
#include "flash.h"
#include "uart.h"
#include "pro212.h"
#include "at24c64.h"
#include "dio.h"

//=====================================
//计算控制需要的数据
//=====================================
void CalcControlData(void)
{
	static int SaveCnt = 0;

	if (++SaveCnt >= 300) {
		SaveCnt = 0;
		//Save_FlowCount(&Flow_Count);
	}
}

//=====================================
//计算统计数据
//=====================================
void CalcStatisData(void)
{
	int i;

	//计算参数值
	for (i = 0; i < 8; ++i) {
		//分钟数据
		if (0 == Ad_MaxMin[i].mnt_cnt) {
			Ad_MaxMin[i].mnt_max = Ad_RealData[i];
			Ad_MaxMin[i].mnt_min = Ad_RealData[i];
		} else {
			if (Ad_RealData[i] > Ad_MaxMin[i].mnt_max)
				Ad_MaxMin[i].mnt_max = Ad_RealData[i];
			if (Ad_RealData[i] < Ad_MaxMin[i].mnt_min)
				Ad_MaxMin[i].mnt_min = Ad_RealData[i];
		}

		Ad_MaxMin[i].mnt_sum += Ad_RealData[i];
		Ad_MaxMin[i].mnt_cnt++;
		Ad_MaxMin[i].mnt_avg = Ad_MaxMin[i].mnt_sum / Ad_MaxMin[i].mnt_cnt;
		Ad_MaxMin[i].mnt_cou += Ad_RealData[i];

		//小时数据
		if (0 == Ad_MaxMin[i].hour_cnt) {
			Ad_MaxMin[i].hour_max = Ad_RealData[i];
			Ad_MaxMin[i].hour_min = Ad_RealData[i];
		} else {
			if (Ad_RealData[i] > Ad_MaxMin[i].hour_max)
				Ad_MaxMin[i].hour_max = Ad_RealData[i];
			if (Ad_RealData[i] < Ad_MaxMin[i].hour_min)
				Ad_MaxMin[i].hour_min = Ad_RealData[i];
		}
		Ad_MaxMin[i].hour_sum += Ad_RealData[i];
		Ad_MaxMin[i].hour_cnt++;
		Ad_MaxMin[i].hour_avg = Ad_MaxMin[i].hour_sum / Ad_MaxMin[i].hour_cnt;
		Ad_MaxMin[i].hour_cou += Ad_RealData[i];

		//日数据
		if (0 == Ad_MaxMin[i].day_cnt) {
			Ad_MaxMin[i].day_max = Ad_RealData[i];
			Ad_MaxMin[i].day_min = Ad_RealData[i];
		} else {
			if (Ad_RealData[i] > Ad_MaxMin[i].day_max)
				Ad_MaxMin[i].day_max = Ad_RealData[i];
			if (Ad_RealData[i] < Ad_MaxMin[i].day_min)
				Ad_MaxMin[i].day_min = Ad_RealData[i];
		}

		Ad_MaxMin[i].day_sum += Ad_RealData[i];
		Ad_MaxMin[i].day_cnt++;
		Ad_MaxMin[i].day_avg = Ad_MaxMin[i].day_sum / Ad_MaxMin[i].day_cnt;
		Ad_MaxMin[i].day_cou += Ad_RealData[i];
	}

	for (i = 0; i < MODBUS_VARNUM; ++i) {
		//分钟数据
		if (0 == Com_MaxMin[i].mnt_cnt) {
			Com_MaxMin[i].mnt_max = Com_Val[i];
			Com_MaxMin[i].mnt_min = Com_Val[i];
		} else {
			if (Com_Val[i] > Com_MaxMin[i].mnt_max)
				Com_MaxMin[i].mnt_max = Com_Val[i];
			if (Com_Val[i] < Com_MaxMin[i].mnt_min)
				Com_MaxMin[i].mnt_min = Com_Val[i];
		}
		Com_MaxMin[i].mnt_sum += Com_Val[i];
		Com_MaxMin[i].mnt_cnt++;
		Com_MaxMin[i].mnt_avg = Com_MaxMin[i].mnt_sum / Com_MaxMin[i].mnt_cnt;
		Com_MaxMin[i].mnt_cou += Com_Val[i];

		//小时数据
		if (0 == Com_MaxMin[i].hour_cnt) {
			Com_MaxMin[i].hour_max = Com_Val[i];
			Com_MaxMin[i].hour_min = Com_Val[i];
		} else {
			if (Com_Val[i] > Com_MaxMin[i].hour_max)
				Com_MaxMin[i].hour_max = Com_Val[i];
			if (Com_Val[i] < Com_MaxMin[i].hour_min)
				Com_MaxMin[i].hour_min = Com_Val[i];
		}
		Com_MaxMin[i].hour_sum += Com_Val[i];
		Com_MaxMin[i].hour_cnt++;
		Com_MaxMin[i].hour_avg = Com_MaxMin[i].hour_sum
				/ Com_MaxMin[i].hour_cnt;
		Com_MaxMin[i].hour_cou += Com_Val[i];

		//日数据
		if (0 == Com_MaxMin[i].day_cnt) {
			Com_MaxMin[i].day_max = Com_Val[i];
			Com_MaxMin[i].day_min = Com_Val[i];
		} else {
			if (Com_Val[i] > Com_MaxMin[i].day_max)
				Com_MaxMin[i].day_max = Com_Val[i];
			if (Com_Val[i] < Com_MaxMin[i].day_min)
				Com_MaxMin[i].day_min = Com_Val[i];
		}

		Com_MaxMin[i].day_sum += Com_Val[i];
		Com_MaxMin[i].day_cnt++;
		Com_MaxMin[i].day_avg = Com_MaxMin[i].day_sum / Com_MaxMin[i].day_cnt;
		Com_MaxMin[i].day_cou += Com_Val[i];
	}
}

//=====================================
//清除统计数据, 
//Cmd为212协议的命令号
//=====================================
void ClearStatisData(int Cmd)
{
	char i;

	if (2051 == Cmd) {
		for (i = 0; i < 8; ++i) {
			Ad_MaxMin[i].mnt_min = 0;
			Ad_MaxMin[i].mnt_max = 0;
			Ad_MaxMin[i].mnt_sum = 0;
			Ad_MaxMin[i].mnt_avg = 0;
			Ad_MaxMin[i].mnt_cou = 0;
			Ad_MaxMin[i].mnt_cnt = 0;
		}
		for (i = 0; i < MODBUS_VARNUM; ++i) {
			Com_MaxMin[i].mnt_min = 0;
			Com_MaxMin[i].mnt_max = 0;
			Com_MaxMin[i].mnt_sum = 0;
			Com_MaxMin[i].mnt_avg = 0;
			Com_MaxMin[i].mnt_cou = 0;
			Com_MaxMin[i].mnt_cnt = 0;
		}
	} else if (2061 == Cmd) {
		for (i = 0; i < 8; ++i) {
			Ad_MaxMin[i].hour_min = 0;
			Ad_MaxMin[i].hour_max = 0;
			Ad_MaxMin[i].hour_sum = 0;
			Ad_MaxMin[i].hour_avg = 0;
			Ad_MaxMin[i].hour_cou = 0;
			Ad_MaxMin[i].hour_cnt = 0;
		}
		for (i = 0; i < MODBUS_VARNUM; ++i) {
			Com_MaxMin[i].hour_min = 0;
			Com_MaxMin[i].hour_max = 0;
			Com_MaxMin[i].hour_sum = 0;
			Com_MaxMin[i].hour_avg = 0;
			Com_MaxMin[i].hour_cou = 0;
			Com_MaxMin[i].hour_cnt = 0;
		}
	} else if (2031 == Cmd) {
		for (i = 0; i < 8; ++i) {
			Ad_MaxMin[i].day_min = 0;
			Ad_MaxMin[i].day_max = 0;
			Ad_MaxMin[i].day_sum = 0;
			Ad_MaxMin[i].day_avg = 0;
			Ad_MaxMin[i].day_cou = 0;
			Ad_MaxMin[i].day_cnt = 0;
		}
		for (i = 0; i < MODBUS_VARNUM; ++i) {
			Com_MaxMin[i].day_min = 0;
			Com_MaxMin[i].day_max = 0;
			Com_MaxMin[i].day_sum = 0;
			Com_MaxMin[i].day_avg = 0;
			Com_MaxMin[i].day_cou = 0;
			Com_MaxMin[i].day_cnt = 0;
		}
	}
}

//=====================================
//判断市电电源状态
//=====================================
int GetPowerStat(void)
{
	char i;
	int sc = DEVICE_FAULT;

	for (i = 0; i < MODBUS_VARNUM; ++i) {
		if (memcmp(Com_Param.mb_var[i].regid, "UAP", 3) == 0) {
			if (Com_Val[i] < 5) {
				sc = DEVICE_STOP;
				break;
			} else {
				sc = DEVICE_RUN;
			}
		} else if (memcmp(Com_Param.mb_var[i].regid, "UBP", 3) == 0) {
			if (Com_Val[i] < 5) {
				sc = DEVICE_STOP;
				break;
			} else {
				sc = DEVICE_RUN;
			}
		} else if (memcmp(Com_Param.mb_var[i].regid, "UCP", 3) == 0) {
			if (Com_Val[i] < 5) {
				sc = DEVICE_STOP;
				break;
			} else {
				sc = DEVICE_RUN;
			}
		}
	}

	return sc;
}

//=====================================
//污染物数据处理函数
//=====================================
void ProcData(void)
{
	static int LastSecond;
	int secnow;

	secnow = Current_Tm.second;
	if (LastSecond == secnow)
		return;

	LastSecond = secnow;

	//每秒计算一次数据
	//CalcControlData();
	CalcStatisData();
	//PWR_Stat = GetPowerStat();
}

#if 0

/////////////////////////////////////
////////////////////////////////////
#define RTDINDEXPAGE	(0ul)
#define MTDINDEXPAGE	(1ul)
#define HTDINDEXPAGE	(2ul)
#define DTDINDEXPAGE	(3ul)

#define INDEXOFFSET	(128ul)

#define RTDSTARTADDR	(10ul)
#define MTDSTARTADDR	(3000ul)
#define HTDSTARTADDR	(5000ul)
#define DTDSTARTADDR	(6000ul)

#define DATAOFFSET		(1ul)

//实时数据存储索引
//实时数据每5分钟存储一包，1小时共12包
typedef struct hisindex_r {
	T_Times timestamp;	//时间戳
	u16 flag[24];//每flag表示一小时的数据，flag中由低到高每bit表示一组数据，1为有数据，0为无数据
}T_RtdIndex, *T_pRtdIndex;

//分钟数据存储索引
//分钟数据每10分钟存储一包，每小时共6包
typedef struct hisindex_m {
	T_Times timestamp;	//时间戳
	u8 flag[24];//每flag表示一小时的数据，flag中由低到高每bit表示一组数据，1为有数据，0为无数据
}T_MtdIndex, *T_pMtdIndex;

//小时数据存储索引,日数据索引
//一天24包数据
typedef struct hisindex_h {
	T_Times timestamp;	//时间戳
	u32 flag;//flag中由低到高每bit表示一组数据，1为有数据，0为无数据
}T_HtdIndex, *T_pHtdIndex;

int str2TM(char *str,T_Times *tm)
{
	int i;

	if(strlen(str)<14)
	return -1;

	for(i = 0;i<14;i++)
	{
		if(!isdigit(str[i]))
		return -2;  //the time string is error,not 0~9
	}

	tm->year = (*(str+2)&0x0f)*10+(*(str+3)&0x0f);
	tm->month = (*(str+4)&0x0f)*10+(*(str+5)&0x0f);
	tm->date = (*(str+6)&0x0f)*10+(*(str+7)&0x0f);
	tm->hour = (*(str+8)&0x0f)*10+(*(str+9)&0x0f);
	tm->minute = (*(str+10)&0x0f)*10+(*(str+11)&0x0f);
	tm->second = (*(str+12)&0x0f)*10+(*(str+13)&0x0f);

	if((tm->month>12)||(tm->date>31)|| (tm->hour>=24)||
			(tm->minute>=60)||(tm->second>=60))
	return -3;

	return 0;
}

char *TM2Str(T_Times *tm)
{
	static char str[16];

	str[0] = '2';
	str[1] = '0';
	str[2] = (tm->year/10)|0x30;
	str[3] = (tm->year%10)|0x30;
	str[4] = (tm->month/10)|0x30;
	str[5] = (tm->month%10)|0x30;
	str[6] = (tm->date/10)|0x30;
	str[7] = (tm->date%10)|0x30;
	str[8] = (tm->hour/10)|0x30;
	str[9] = (tm->hour%10)|0x30;
	str[10] = (tm->minute/10)|0x30;
	str[11] = (tm->minute%10)|0x30;
	str[12] = (tm->second/10)|0x30;
	str[13] = (tm->second%10)|0x30;
	str[14] = '\0';

	return str;
}

int timeCmp(T_Times *tm1,T_Times *tm2)
{
	char str[16] = "\0";

	strcpy(str,TM2Str(tm1));
	return strcmp(str,TM2Str(tm2));
}

//清除历史数据存储信息
void resetFlashIndexPage(void)
{
	u32 addr;
	//for(int i=0;i<7;i++)
	{
		addr = FLASHADDR(RTDINDEXPAGE,0);
		FlashClearMemory(addr,1024,FLASHBUFFER1);
		addr = FLASHADDR(MTDINDEXPAGE,0);
		FlashClearMemory(addr,1024,FLASHBUFFER1);
		addr = FLASHADDR(HTDINDEXPAGE,0);
		FlashClearMemory(addr,1024,FLASHBUFFER1);
		addr = FLASHADDR(DTDINDEXPAGE,0);
		FlashClearMemory(addr,1024,FLASHBUFFER1);
	}
}

/**************************************************************
 *                save history data in flash
 *func：
 *	saveRtdData: save realdata in flash
 *	saveMtdData: save minute data in flash
 *	saveHtdData: save hour data in flash
 *	saveDtdData: save day data in flash
 *
 *param：
 *	tm:		timestamp
 *	dat:	saved message bag
 *return：
 *	none
 *author：	Wen Jianjun
 *date：		2010-10-22
 **************************************************************/
//test wen 2010-11-02
//void testIndex(T_RtdIndex *index,T_Times *tm);
//end test wen 2010-11-02
void saveRtdData(T_Times *tm,char *dat)
{
	T_RtdIndex rtdindex;
	u32 addr;

	//read index
	addr = FLASHADDR(RTDINDEXPAGE,(tm->weekday-1)*INDEXOFFSET);
	FlashMemorytoBuffer(addr,FLASHBUFFER1);
	FlashBufferRead(addr,&rtdindex,sizeof(T_RtdIndex),FLASHBUFFER1);

	//test wen 2010-11-02
	//SendData("\r\nformer\r\n",strlen("\r\nformer\r\n"),GPRS);
	//testIndex(&rtdindex,tm);
	//end test wen 2010-11-02

	//FlashMemoryRead(addr,&index,sizeof(T_RtdIndex));
	if((tm->hour == 0)&&((tm->minute/5)==0))
	{
		memset(&rtdindex,0,sizeof(T_RtdIndex));
	}
	if((tm->year != rtdindex.timestamp.year) || (tm->month != rtdindex.timestamp.month) ||
			(tm->date != rtdindex.timestamp.date))
	{
		memset(&rtdindex,0,sizeof(T_RtdIndex));
	}
	rtdindex.timestamp.year = tm->year;
	rtdindex.timestamp.month = tm->month;
	rtdindex.timestamp.date = tm->date;
	rtdindex.timestamp.hour = 0;
	rtdindex.timestamp.minute = 0;
	rtdindex.timestamp.second = 0;
	rtdindex.timestamp.weekday = tm->weekday;
	//modify index
	rtdindex.flag[tm->hour] |= (0x0001<<(tm->minute/5));

	//test wen 2010-11-02
	//testIndex(&rtdindex,tm);
	//end test wen 2010-11-02

	//save index
	FlashBufferWrite(addr,&rtdindex,sizeof(T_RtdIndex),FLASHBUFFER1);
	FlashBuffertoMemory(addr,FLASHBUFFER1);
	//FlashMemoryWrite(addr,&index,sizeof(T_RtdIndex),FLASHBUFFER1);
	//save data
	addr = FLASHADDR((RTDSTARTADDR + (tm->weekday-1)*288 + tm->hour*12 + tm->minute/5),0);
	FlashMemoryWrite(addr,dat,strlen(dat)+1,FLASHBUFFER2);
}

void saveMtdData(T_Times *tm,char *dat)
{
	T_MtdIndex mtdindex;
	u32 addr;

	//read index
	addr = FLASHADDR(MTDINDEXPAGE,(tm->weekday-1)*INDEXOFFSET);
	FlashMemorytoBuffer(addr,FLASHBUFFER1);
	FlashBufferRead(addr,&mtdindex,sizeof(T_MtdIndex),FLASHBUFFER1);
	//FlashMemoryRead(addr,&index,sizeof(T_MtdIndex));
	if((tm->hour == 0)&&(tm->minute == 0))
	{
		memset(&mtdindex,0,sizeof(T_MtdIndex));	//clear timestamp
	}
	if((tm->year != mtdindex.timestamp.year) || (tm->month != mtdindex.timestamp.month) ||
			(tm->date != mtdindex.timestamp.date))
	{
		memset(&mtdindex,0,sizeof(T_MtdIndex));
	}
	//set timstamp
	mtdindex.timestamp.year = tm->year;
	mtdindex.timestamp.month = tm->month;
	mtdindex.timestamp.date = tm->date;
	mtdindex.timestamp.hour = 0;
	mtdindex.timestamp.minute = 0;
	mtdindex.timestamp.second = 0;
	mtdindex.timestamp.weekday = tm->weekday;

	//modify index
	mtdindex.flag[tm->hour] |= (0x01<<(tm->minute/10));

	//save index
	FlashBufferWrite(addr,&mtdindex,sizeof(T_MtdIndex),FLASHBUFFER1);
	FlashBuffertoMemory(addr,FLASHBUFFER1);
	//FlashMemoryWrite(addr,&index,sizeof(T_MtdIndex),FLASHBUFFER1);
	//save data
	addr = FLASHADDR((MTDSTARTADDR + (tm->weekday-1)*144 + tm->hour*6 + tm->minute/10),0);
	FlashMemoryWrite(addr,dat,strlen(dat)+1,FLASHBUFFER2);
}

void saveHtdData(T_Times *tm,char *dat)
{
	T_HtdIndex htdindex;
	u32 addr;

	//read index
	addr = FLASHADDR(HTDINDEXPAGE,(tm->weekday-1)*INDEXOFFSET);
	FlashMemorytoBuffer(addr,FLASHBUFFER1);
	FlashBufferRead(addr,&htdindex,sizeof(T_HtdIndex),FLASHBUFFER1);
	//FlashMemoryRead(addr,&index,sizeof(T_HtdIndex));
	if(tm->hour == 0)
	{
		memset(&htdindex,0,sizeof(T_HtdIndex));	//clear timestamp
	}
	if((tm->year != htdindex.timestamp.year) || (tm->month != htdindex.timestamp.month) ||
			(tm->date != htdindex.timestamp.date))
	{
		memset(&htdindex,0,sizeof(T_HtdIndex));
	}

	//set timstamp
	htdindex.timestamp.year = tm->year;
	htdindex.timestamp.month = tm->month;
	htdindex.timestamp.date = tm->date;
	htdindex.timestamp.hour = 0;
	htdindex.timestamp.minute = 0;
	htdindex.timestamp.second = 0;
	htdindex.timestamp.weekday = tm->weekday;

	//modify index
	htdindex.flag |= (((u32)1)<<(tm->hour));//2010-12-17

	//save index
	FlashBufferWrite(addr,&htdindex,sizeof(T_HtdIndex),FLASHBUFFER1);
	FlashBuffertoMemory(addr,FLASHBUFFER1);
	//FlashMemoryWrite(addr,&index,sizeof(T_HtdIndex),FLASHBUFFER1);

	//save data
	addr = FLASHADDR((HTDSTARTADDR + (tm->weekday-1)*24 + tm->hour),0);
	FlashMemoryWrite(addr,dat,strlen(dat)+1,FLASHBUFFER2);
}

void saveDtdData(T_Times *tm,char *dat)
{
	T_HtdIndex dtdindex;
	u32 addr;

	//read index
	addr = FLASHADDR(DTDINDEXPAGE,(tm->weekday-1)*INDEXOFFSET);
	FlashMemorytoBuffer(addr,FLASHBUFFER1);
	FlashBufferRead(addr,&dtdindex,sizeof(T_HtdIndex),FLASHBUFFER1);
	//FlashMemoryRead(addr,&index,sizeof(T_HtdIndex));

	memset(&dtdindex,0,sizeof(T_HtdIndex));//clear timestamp

	//set timstamp
	dtdindex.timestamp.year = tm->year;
	dtdindex.timestamp.month = tm->month;
	dtdindex.timestamp.date = tm->date;
	dtdindex.timestamp.hour = 0;
	dtdindex.timestamp.minute = 0;
	dtdindex.timestamp.second = 0;
	dtdindex.timestamp.weekday = tm->weekday;

	//modify index
	dtdindex.flag = 1;

	//save index
	FlashBufferWrite(addr,&dtdindex,sizeof(T_HtdIndex),FLASHBUFFER1);
	FlashBuffertoMemory(addr,FLASHBUFFER1);
	//FlashMemoryWrite(addr,&index,sizeof(T_HtdIndex),FLASHBUFFER1);

	//save data
	addr = FLASHADDR((DTDSTARTADDR + (tm->weekday-1)),0);
	FlashMemoryWrite(addr,dat,strlen(dat)+1,FLASHBUFFER2);
}

/**************************************************************
 *                get history data from flash
 *func：
 *	checkoutRtdData: get realdata from flash
 *	checkoutMtdData: get minute data from flash
 *	checkoutHtdData: get hour data from flash
 *	checkoutDtdData: get day data from flash
 *
 *param：
 *	start:	start time
 *	end:	end time
 *   buf:	message bag
 *return：
 *	1 has valid message
 *	0 no more valid message
 *author：	Wen Jianjun
 *date：		2010-10-22
 **************************************************************/
int checkoutRtdData(T_Times *start,char *buf)
{
	T_RtdIndex index;
	char i,j;
	u32 addr;
	int counter;
	T_Times starttmp;

	memcpy(&starttmp,start,sizeof(T_Times));
	starttmp.hour = 0;
	starttmp.minute = 0;
	starttmp.second = 0;

	for(i=0;i<7;i++)
	{
		addr = FLASHADDR(RTDINDEXPAGE,i*INDEXOFFSET);
		FlashMemoryRead(addr,&index,sizeof(T_RtdIndex));

		if(timeCmp(&starttmp,&(index.timestamp))==0)
		{
			for(j=0;j<12;j++)
			{
				if((index.flag[start->hour]>>j)&0x00001)
				{
					addr = FLASHADDR((RTDSTARTADDR + (index.timestamp.weekday-1)*288 + start->hour*12 + j),0);
					FlashMemoryRead(addr,buf,512);
					Uart_SendData((u8 *)buf,strlen(buf),GPRS_COM);
					counter++;
				}
			}
			break;
		}
	}
	return counter;
}

int checkoutMtdData(T_Times *start,T_Times *end,char *buf)
{
	T_MtdIndex index;
	u32 addr;
	int counter = 0;
	char i=0,record;
	int j;
	T_Times starttmp;

	memcpy(&starttmp,start,sizeof(T_Times));
	starttmp.hour = 0;
	starttmp.minute = 0;
	starttmp.second = 0;

	for(i=0;i<7;i++)
	{
		addr = FLASHADDR(MTDINDEXPAGE,i*INDEXOFFSET);
		FlashMemoryRead(addr,&index,sizeof(T_MtdIndex));

		if(timeCmp(&starttmp,&(index.timestamp))==0)
		{
			for(j=start->hour;j<=end->hour;j++)
			{
				for(record=0;record<6;record++)
				{
					if((index.flag[j]>>record)&0x01)
					{
						addr = FLASHADDR((MTDSTARTADDR + (index.timestamp.weekday-1)*144 + j*6 + record),0);
						FlashMemoryRead(addr,buf,512);
						//send
						Uart_SendData((u8 *)buf,strlen(buf),GPRS_COM);
						counter++;
					}
				}
			}
		}
	}
	return counter;
}

int checkoutHtdData(T_Times *start,T_Times *end,char *buf)
{
	T_HtdIndex chtdindex;
	u32 addr;
	int counter = 0;
	char i=0;
	int j;
	T_Times starttmp,endtmp;

	memcpy(&starttmp,start,sizeof(T_Times));
	starttmp.hour = 0;
	starttmp.minute = 0;
	starttmp.second = 0;
	memcpy(&endtmp,end,sizeof(T_Times));
	endtmp.hour = 0;
	endtmp.minute = 0;
	endtmp.second = 0;

	for(i=0;i<7;i++)
	{
		addr = FLASHADDR(HTDINDEXPAGE,i*INDEXOFFSET);
		FlashMemoryRead(addr,&chtdindex,sizeof(T_HtdIndex));

		if(timeCmp(&starttmp,&(chtdindex.timestamp))==0)
		{
			for(j=start->hour;j<=end->hour;j++)
			{
				if((chtdindex.flag>>j)&0x00000001)
				{
					addr = FLASHADDR((HTDSTARTADDR + (chtdindex.timestamp.weekday-1)*24 + j),0);
					FlashMemoryRead(addr,buf,512);
					//send
					Uart_SendData((u8 *)buf,strlen(buf),GPRS_COM);
					counter++;
				}
			}
		}
	}
	return counter;
}

int checkoutDtdData(T_Times *start,T_Times *end,char *buf)
{
	T_HtdIndex cdtdindex;
	u32 addr;
	char i = 0;
	int counter = 0;

	for(i=0;i<7;i++)
	{
		addr = FLASHADDR(DTDINDEXPAGE,i*INDEXOFFSET);
		FlashMemoryRead(addr,&cdtdindex,sizeof(T_HtdIndex));
		if((timeCmp(&(cdtdindex.timestamp),start)>=0)
				&&(timeCmp(&(cdtdindex.timestamp),end)<=0))
		{
			if(cdtdindex.flag == 1)
			{
				addr = FLASHADDR((DTDSTARTADDR + i),0);
				FlashMemoryRead(addr,buf,512);
				Uart_SendData((u8 *)buf,strlen(buf),GPRS_COM);
				counter++;
			}
		}
	}
	return counter;
}

#endif //end of #if 0
