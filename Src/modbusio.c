
#include "system.h"
#include "modbusio.h"
#include "sc16is752.h"
#include "dio.h"
#include <stdlib.h>
#include <string.h>

MBIO_PARAM MbIO_Param;

extern const char ComBaud_Str[4][6];
char MbIO_Err[MODBUSIO_VARNUM];

//=====================================
//TimerA的定时器10ms产生一次中断
//在每次中断中加入一个计数器，
//每10ms计数器加1，用于modbus的超时判断
//=====================================
#define MBIOOTCNTNUM (MS2TENMS(1000)) 	//modbus超时值的计数器
#define MBIODELAYCNTNUM (MS2TENMS(150)) //两个modbus设备之间的延时
INT8U MbIOOtCnt;			        	//modbus超时计数器
INT8U MbIODelayCnt;

#define  MBIOMAXERRCNT 3		    	//modbus最大错误计数，超过此值后会将数据清零
//=====================================
//Modbus通讯中用到的全局变量
//=====================================
static unsigned char MBIOCurrentDev;		//modbus当前正处理的设备编号
static unsigned char MBIOState;			//modbus当前的处理状态
static unsigned char MBIORcvDataLen;		//modbus要接收的应答数据长度
static unsigned char MBIORcvDataBuf[32];	//modbus接收的应答数据存放的缓冲区
static unsigned char MBIORcved;			//modbus已接收的应答数据长度

//=====================================
//外扩IO的默认采集函数
//=====================================
void Default_MbIOParam(MBIO_PARAM *Param)
{
	Param->baud_index = 0;

	Param->devid[0] = 1;
	Param->funccode[0] = 2;
	Param->regaddr[0] = 1;
	Param->reglen[0] = 16;

	Param->devid[1] = 2;
	Param->funccode[1] = 15;
	Param->regaddr[1] = 17;
	Param->reglen[1] = 8;
}
//=====================================
//modbus初始化
//=====================================
void ModbusIO_Init(void)
{
	Default_MbIOParam(&MbIO_Param);
	IS752_ChangeBaudRate(atoi(ComBaud_Str[MbIO_Param.baud_index]),
			MODBUSIO_COM);
}

//=====================================
//modbus crc计算
//=====================================
extern unsigned short CRC16_Modbus(unsigned char *src, int len);

//=====================================
//modbus 命令发送
//=====================================
void SendModbusIOCmd(void)
{
	unsigned int regaddr, reglen;
	unsigned char devaddr, funccode;
	int uartchannel;
	unsigned char modbus_bag[22];
	unsigned short crc;
	unsigned char len, doval;

	uartchannel = MODBUSIO_COM;
	devaddr = MbIO_Param.devid[MBIOCurrentDev];
	funccode = MbIO_Param.funccode[MBIOCurrentDev];
	regaddr = MbIO_Param.regaddr[MBIOCurrentDev] - 1;
	reglen = MbIO_Param.reglen[MBIOCurrentDev];

	if (funccode == 2) {
		//设备地值
		modbus_bag[0] = devaddr;
		//命令号
		modbus_bag[1] = funccode;
		//modbus起始地址
		//高位地址
		modbus_bag[2] = (regaddr & 0xFF00) >> 8;
		//低位地址
		modbus_bag[3] = regaddr & 0x00FF;
		//字节个数
		modbus_bag[4] = (reglen & 0xFF00) >> 8;
		modbus_bag[5] = reglen & 0x00FF;
		//数据段校验码
		crc = CRC16_Modbus(modbus_bag, 6);
		modbus_bag[6] = (crc & 0xFF00) >> 8;	//高字节
		modbus_bag[7] = crc & 0x00FF;	//低字节
		len = 8;

		if (reglen % 8 == 0)
			MBIORcvDataLen = 5 + reglen / 8;
		else
			MBIORcvDataLen = 5 + reglen / 8 + 1;
	} else	// if(funccode == 15)
	{
		//设备地值
		modbus_bag[0] = devaddr;
		//命令号
		modbus_bag[1] = funccode;
		//modbus起始地址
		//高位地址
		modbus_bag[2] = (regaddr & 0xFF00) >> 8;
		//低位地址
		modbus_bag[3] = regaddr & 0x00FF;
		//字节个数
		modbus_bag[4] = (reglen & 0xFF00) >> 8;
		modbus_bag[5] = reglen & 0x00FF;
		//后续字节数，目前固定为1
		modbus_bag[6] = 1;
		//IO的输出数据
		doval = (unsigned char) (DO_Val >> 8);
		modbus_bag[7] = doval;
		//数据段校验码
		crc = CRC16_Modbus(modbus_bag, 8);
		modbus_bag[8] = (crc & 0xFF00) >> 8;	//高字节
		modbus_bag[9] = crc & 0x00FF;	//低字节
		len = 10;

		MBIORcvDataLen = 8;
	}

	//发送协议包
	Uart_ClearRcvBuf(uartchannel);
	Uart_SendData(modbus_bag, len, uartchannel);

	MbIOOtCnt = 0;
	MBIORcved = 0;
}
//=====================================
//modbus 命令应答接收函数, 返回-1表示接收超时，0表示继续接收，1表示接收完成
//=====================================
int RcvModbusIORsp(void)
{
	int ret, uartchannel;

	uartchannel = MODBUSIO_COM;

	while (1) {
		ret = Uart_RcvData(&MBIORcvDataBuf[MBIORcved], 1, uartchannel);
		if (ret < 1) {
			if (MbIOOtCnt >= MBIOOTCNTNUM)
				return -1;
			else
				return 0;
		}

		if (++MBIORcved >= MBIORcvDataLen)	//数据接收完成
			return 1;
	}
}
//=====================================
//modbus 命令应答解析函数, 返回-1表示数据错误，0表示数据已正确处理
//=====================================
int AnalystModbusIORsp(void)
{
	unsigned char devaddr, funccode;
	unsigned short crc, crc2;
	INT16U dival;

	devaddr = MbIO_Param.devid[MBIOCurrentDev];
	funccode = MbIO_Param.funccode[MBIOCurrentDev];

	if (MBIORcvDataBuf[0] != devaddr)
		return -1;

	if (MBIORcvDataBuf[1] != funccode)
		return -1;

	crc = CRC16_Modbus(MBIORcvDataBuf, MBIORcvDataLen - 2);
	crc2 = (MBIORcvDataBuf[MBIORcvDataLen - 2] << 8)
			+ MBIORcvDataBuf[MBIORcvDataLen - 1];
	if (crc != crc2)
		return -1;

	if (funccode == 2) {
		if (MBIORcvDataBuf[2] == 1)
			dival = MBIORcvDataBuf[3];
		else
			dival = MBIORcvDataBuf[3] + ((INT16U) MBIORcvDataBuf[4] << 8);
		DI_Val = (DI_Val & 0x0000ffff) + ((INT32U) (dival) << 16);
	} else	// if(funccode == 15)
	{

	}

	MbIO_Err[MBIOCurrentDev] = 0;

	return 0;
}
//=====================================
//modbus 命令处理函数
//=====================================
void ProcessModbusIO(void)
{
	int flag, ret;

	flag = 0;
	while (1) {
		switch (MBIOState) {
		case 0:		//发送命令
		{
			if (++MbIO_Err[MBIOCurrentDev] > MBIOMAXERRCNT) {
				MbIO_Err[MBIOCurrentDev] = 0;
			}

			SendModbusIOCmd();
			MBIOState = 1;
			flag = 1;
		}
			break;
		case 1:		//接收应答
			ret = RcvModbusIORsp();
			if (0 == ret) {
				flag = 1;
			} else if (-1 == ret) {
				MBIOState = 3;
			} else {
				MBIOState = 2;
			}
			break;
		case 2:		//处理应答
			ret = AnalystModbusIORsp();
			if (-1 == ret) {
			}
			MBIOState = 3;
			MbIODelayCnt = 0;
			break;
		case 3:		//切换设备
			++MBIOCurrentDev;
			if (MBIOCurrentDev < MODBUSIO_VARNUM)	//继续扫描下一个设备
			{
				MBIOState = 0xff;
				flag = 1;
			} else						//等待直到一个扫描周期结束再进入下一个扫描周期
			{
				MBIOCurrentDev = 0;
				MBIOState = 0xff;
				flag = 1;
			}
			break;
		case 0xff: {
			if (MbIODelayCnt >= MBIODELAYCNTNUM)
				MBIOState = 0;
			flag = 1;
		}
			break;
		default:
			flag = 1;
			break;
		}

		if (1 == flag)
			break;
	}
}

