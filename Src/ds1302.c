#include "system.h"

/**********************************************************************************************************
 实时时钟时钟控制信号线定义
 **********************************************************************************************************/
/*————控制信号线定义————————*/

#define RST10320  P3OUT&=~BIT5
#define RST10321  P3OUT|=BIT5
#define IO1032W   P3OUT&=~BIT7
#define IO1032R   P3OUT|=BIT7
#define CLK10320  P3OUT&=~BIT6
#define CLK10321  P3OUT|=BIT6

#define IO1032IN  P3DIR&=~BIT7
#define IO1032OUT P3DIR|=BIT7
#define IO1032VAL P3IN&BIT7

#define DELAYNUM 2

void delays(INT16U m)
{
	INT16U n;
	n = 0;
	while (n < m)
		n++;
}
/*
 *********************************************************************************************************
 *                           实时时钟DS1302函数
 *	说 明：	实时时钟DS1302函数
 *参数说明：
 *		数据顺序：秒 分 时 日 月 星期 年
 *
 *********************************************************************************************************
 */
/*---------------------向实时时钟DS1302输入一个字节子程序------------------------*/
void v_RTInputByte(INT8U ucDa) //reentrant
{
	INT8U i;
	INT8U AC = ucDa;

	IO1032OUT;
	delays(DELAYNUM);
	CLK10320;                   //T_CLK = 0;

	//八次循环
	for (i = 8; i > 0; i--) {
		//最低位是否==1
		if ((AC & 0x01)) {
			IO1032R;            //T_IO = 1;	最低位=1
		} else {
			IO1032W;            //T_IO = 0; 最低位=0
		}
		//delays(DELAYNUM);
		CLK10321;               //T_CLK = 1;
		delays(DELAYNUM);
		CLK10320;               //T_CLK = 0;
		delays(DELAYNUM);
		AC = AC >> 1; 			//等于向右移一位
	}
	//delays(DELAYNUM);
}

/*---------------------向实时时钟DS1302某寄存器写一个字节------------------------*/
void v_W1302(INT8U ucAddr, INT8U ucDa)
{
	RST10320;                   //T_RST = 0;
	delays(DELAYNUM);
	CLK10320;                   //T_CLK = 0;
	delays(DELAYNUM);
	RST10321;                   //T_RST = 1;
	delays(DELAYNUM);
	v_RTInputByte(ucAddr);      //写地址，命令
	v_RTInputByte(ucDa);        //写1Byte数据
	CLK10320;                   //T_CLK = 0;
	delays(DELAYNUM);
	RST10320;                   //T_RST =0;
	delays(DELAYNUM);
}

/*---------------------从实时时钟DS1302读取一个字节子程序------------------------*/
INT8U uc_RTOutputByte(void)
{
	INT8U i;
	INT8U AC;

	IO1032IN;
	delays(DELAYNUM);
	for (i = 8; i > 0; i--) {
		AC = AC >> 1; 	        //等于向右移一位
		if ((IO1032VAL) == 0)           // T_IO == 0
			AC = AC & 0x7f;
		else
			AC = AC | 0x80;
		//delays(DELAYNUM);
		CLK10321;               //T_CLK = 1;
		delays(DELAYNUM);
		CLK10320;               //T_CLK = 0;
		delays(DELAYNUM);
	}
	return (AC);
}

/*---------------------从实时时钟DS1302指定地址读取一个字节子程序------------------------*/
INT8U uc_R1302(INT8U ucAddr)
{
	INT8U ucDa;

	RST10320;                   //T_RST = 0;
	delays(DELAYNUM);
	CLK10320;                   //T_CLK = 0;
	delays(DELAYNUM);
	RST10321;                   //T_RST = 1;
	v_RTInputByte(ucAddr);      //地址，命令
	ucDa = uc_RTOutputByte();   //读1Byte数据
	CLK10321;                   //T_CLK = 1;
	delays(DELAYNUM);
	RST10320;                   //T_RST =0;
	delays(DELAYNUM);
	return (ucDa);
}

/*---------------------设置充电模式------------------------------*/
void v_charge1302(void)
{
	v_W1302(0x8e, 0x00);         //控制命令,WP=0,写操作
	v_W1302(0x90, 0xc5);         //one diode,2K register Ima=(3.3-0.7)/2K ~1.2mA
	v_W1302(0x8e, 0x80);         //控制命令,WP=1,写保护
}

/*---------------------设置实时时钟DS1302------------------------*/
int CalcWeekday(int year, int month, int day)
{
	int week, tp;

	tp = 0;
	week = 5;                 //初始化2000-01-01星期六
	while (tp != year) {                       //处理年(2000开始)
		week++;             //是上一年的星期的下一天 公式:365%7=1
		if (tp % 4 == 0)
			week++;  //判断是否是閏年,閏年2月是29天 公式:366%7=2
		tp++;           //下一年的1月1日
	}

	tp = 1;
	while (tp != month) {       //处理月(1月开始)
		switch (tp) {
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			week = week + 3;
			break;          //大月天数31%7
		case 2:
			if (year % 4 == 0)
				week++;
			break;          //閏月天数,閏年2月是29天 公式:366%7=2 29%7
		case 4:
		case 6:
		case 9:
		case 11:
			week = week + 2;
			break;      //小月天数30%7
		}
		tp++;           //处理月
	}

	week = week + day;      //处理日(從1日开始)
	week = week % 7;
	if (week == 0)
		week = 7;

	return week;        //返回星期
}

void v_Set1302(INT8U *System_Time)
{
	INT8U i;
	INT8U hour, minute, second;
	INT8U year, month, date;
	INT8U pSecDa[7];
	INT8U ucAddr = 0x80;

	year = ((System_Time[2] << 4) & 0xf0) | (System_Time[3] & 0x0f);
	month = ((System_Time[4] << 4) & 0xf0) | (System_Time[5] & 0x0f);
	date = ((System_Time[6] << 4) & 0xf0) | (System_Time[7] & 0x0f);
	hour = ((System_Time[8] << 4) & 0xf0) | (System_Time[9] & 0x0f);
	minute = ((System_Time[10] << 4) & 0xf0) | (System_Time[11] & 0x0f);
	second = ((System_Time[12] << 4) & 0xf0) | (System_Time[13] & 0x0f);
	second = second & 0x7f;

	pSecDa[0] = second;
	pSecDa[1] = minute;
	pSecDa[2] = hour;
	pSecDa[3] = date;
	pSecDa[4] = month;
	pSecDa[6] = year;
	pSecDa[5] = CalcWeekday(year, month, date);
	v_W1302(0x8e, 0x00);             //控制命令,WP=0,写操作
	for (i = 0; i < 6; i++) {
		v_W1302(ucAddr, pSecDa[i]);  //秒 分 时 日 月 星期 年
		ucAddr += 2;
	}
	v_W1302(ucAddr, pSecDa[6]);      //秒 分 时 日 月 星期 年
	v_W1302(0x8e, 0x80);             //控制命令,WP=1,写保护
}

/*---------------------从实时时钟DS1302中读取时间------------------------*/
//数据顺序：秒 分 时 日 月 星期 年
void v_Get1302(INT8U *System_Time, T_Times *Tm)
{
	INT8U ucCurtime[7];
	INT8U i;
	INT8U ucAddr = 0x81;

	for (i = 0; i < 7; i++) {
		ucCurtime[i] = uc_R1302(ucAddr);/*格式为: 秒 分 时 日 月 星期 年 */
		ucAddr += 2;
	}

	System_Time[0] = 0x32;					        //年的千位
	System_Time[1] = 0x30;							//年的百位
	System_Time[2] = (ucCurtime[6] >> 4) & 0x0f | 0x30;	//年的十位
	System_Time[3] = ucCurtime[6] & 0x0f | 0x30;		//年的个位
	System_Time[4] = (ucCurtime[4] >> 4) & 0x0f | 0x30;	//月的十位
	System_Time[5] = ucCurtime[4] & 0x0f | 0x30;		//月的个位
	System_Time[6] = (ucCurtime[3] >> 4) & 0x0f | 0x30;	//日的十位
	System_Time[7] = ucCurtime[3] & 0x0f | 0x30;		//日的个位
	System_Time[8] = (ucCurtime[2] >> 4) & 0x0f | 0x30;	//时的十位
	System_Time[9] = ucCurtime[2] & 0x0f | 0x30;		//时的个位
	System_Time[10] = (ucCurtime[1] >> 4) & 0x0f | 0x30;	//分的十位
	System_Time[11] = ucCurtime[1] & 0x0f | 0x30;		//分的个位
	System_Time[12] = (ucCurtime[0] >> 4) & 0x0f | 0x30;	//秒的十位
	System_Time[13] = ucCurtime[0] & 0x0f | 0x30;		//秒的个位
	System_Time[14] = '\0';

	Tm->second = ((ucCurtime[0] >> 4) & 0x07) * 10 + (ucCurtime[0] & 0x0f);
	Tm->minute = ((ucCurtime[1] >> 4) & 0x0f) * 10 + (ucCurtime[1] & 0x0f);
	Tm->hour = ((ucCurtime[2] >> 4) & 0x0f) * 10 + (ucCurtime[2] & 0x0f);
	Tm->date = ((ucCurtime[3] >> 4) & 0x0f) * 10 + (ucCurtime[3] & 0x0f);
	Tm->month = ((ucCurtime[4] >> 4) & 0x0f) * 10 + (ucCurtime[4] & 0x0f);
	Tm->weekday = ucCurtime[5] & 0x07;
	Tm->year = ((ucCurtime[6] >> 4) & 0x0f) * 10 + (ucCurtime[6] & 0x0f);
}

