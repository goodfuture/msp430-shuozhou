#include "system.h"
#include "modbus.h"
#include "sc16is752.h"
#include <stdlib.h>
#include <string.h>

COM_PARAM Com_Param;

const char ComBaud_Str[4][6] = { "9600", "19200", "2400", "4800" };
float Com_Val[MODBUS_VARNUM];
char Com_Err[MODBUS_VARNUM];
MAX_MIN_DATA Com_MaxMin[MODBUS_VARNUM];

//=====================================
//TimerA�Ķ�ʱ��10ms����һ���ж�
//��ÿ���ж��м���һ����������
//ÿ10ms��������1������modbus�ĳ�ʱ�ж�
//=====================================
#define  MBOTCNTNUM	    (MS2TENMS(1000)) //modbus��ʱֵ�ļ�����
INT8U MbOtCnt;			        //modbus��ʱ������

#define  MBSCANCNTNUM	(S2TENMS(2))    //modbusɨ�����ڵļ�����
INT16U MbScanCnt;	            //modbusɨ�����ڼ�����

#define  MBMAXERRCNT 3		    //modbus�����������������ֵ��Ὣ��������
//=====================================
//ModbusͨѶ���õ���ȫ�ֱ���
//=====================================
static unsigned char MBCurrentDev;		//modbus��ǰ���������豸���
static unsigned char MBState;			//modbus��ǰ�Ĵ���״̬
static unsigned char MBRcvDataLen;		//modbusҪ���յ�Ӧ�����ݳ���
static unsigned char MBRcvDataBuf[32];	//modbus���յ�Ӧ�����ݴ�ŵĻ�����
static unsigned char MBRcved;			//modbus�ѽ��յ�Ӧ�����ݳ���

//=====================================
//modbus��ʼ��
//=====================================
void Modbus_Init(void)
{
	IS752_ChangeBaudRate(atoi(ComBaud_Str[Com_Param.baud_index]), MODBUS_COM);
	memset(Com_Val, 0, sizeof(Com_Val));
	MBState = 0;
}

//=====================================
//modbus crc����
//=====================================
const unsigned char auchCRCHi[256] = { 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80,
		0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
		0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40 };

const unsigned char auchCRCLo[256] = { 0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02,
		0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D,
		0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08,
		0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF,
		0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16,
		0xD6, 0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31,
		0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34,
		0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B,
		0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A,
		0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25,
		0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20,
		0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7,
		0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E,
		0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9,
		0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC,
		0x7C, 0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3,
		0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52,
		0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D,
		0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58,
		0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F,
		0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46,
		0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40 };

unsigned short CRC16_Modbus(unsigned char *src, int len)
{
	int j;
	int uIndex; /* will index into CRC lookup table*/
	unsigned char des[2];

	des[0] = 0xff;
	des[1] = 0xff;
	for (j = 0; j < len; j++) {
		uIndex = des[0] ^ *src++; /* calculate the CRC */
		des[0] = des[1] ^ auchCRCHi[uIndex];
		des[1] = auchCRCLo[uIndex];
	}

	return ((((unsigned short) des[0]) << 8) | des[1]);
}
//=====================================
//modbus �����
//=====================================
void SendModbusCmd(void)
{
	unsigned int regaddr;
	unsigned char devaddr;
	int uartchannel;
	unsigned char modbus_bag[22];
	unsigned short crc;
	unsigned char len;

	uartchannel = MODBUS_COM;
	devaddr = Com_Param.mb_var[MBCurrentDev].devid;
	regaddr = Com_Param.mb_var[MBCurrentDev].regaddr - 1;

	//�豸��ֵ
	modbus_bag[0] = devaddr;
	//�����
	modbus_bag[1] = 0x03;
	//modbus��ʼ��ַ
	//��λ��ַ
	modbus_bag[2] = (regaddr & 0xFF00) >> 8;
	//��λ��ַ
	modbus_bag[3] = regaddr & 0x00FF;
	//�ֽڸ���
	if (Com_Param.mb_var[MBCurrentDev].type == 0) {
		modbus_bag[4] = 0x00;
		modbus_bag[5] = 0x01;
		MBRcvDataLen = 7;
	} else {
		modbus_bag[4] = 0x00;
		modbus_bag[5] = 0x02;
		MBRcvDataLen = 9;
	}
	//���ݶ�У����
	crc = CRC16_Modbus(modbus_bag, 6);
	modbus_bag[6] = (crc & 0xFF00) >> 8;	//���ֽ�
	modbus_bag[7] = crc & 0x00FF;	//���ֽ�
	len = 8;

	//����Э���
	Uart_ClearRcvBuf(uartchannel);
	Uart_SendData(modbus_bag, len, uartchannel);

	MbOtCnt = 0;
	MBRcved = 0;
}
//=====================================
//modbus ����Ӧ����պ���, ����-1��ʾ���ճ�ʱ��0��ʾ�������գ�1��ʾ�������
//=====================================
int RcvModbusRsp(void)
{
	int ret, uartchannel;

	uartchannel = MODBUS_COM;

	while (1) {
		ret = Uart_RcvData(&MBRcvDataBuf[MBRcved], 1, uartchannel);
		if (ret < 1) {
			if (MbOtCnt >= MBOTCNTNUM)
				return -1;
			else
				return 0;
		}

		if (++MBRcved >= MBRcvDataLen)	//���ݽ������
			return 1;
	}
}
//=====================================
//modbus ����Ӧ���������, ����-1��ʾ���ݴ���0��ʾ��������ȷ����
//=====================================
int AnalystModbusRsp(void)
{
	unsigned char devaddr;
	unsigned short crc, crc2;
	INT32U d1, d2;
	INT32U data;
	union {
		unsigned char byte[4];
		float data;
	} pfdata;

	devaddr = Com_Param.mb_var[MBCurrentDev].devid;

	if (MBRcvDataBuf[0] != devaddr)
		return -1;

	if (MBRcvDataBuf[1] != 0x03)
		return -1;

	crc = CRC16_Modbus(MBRcvDataBuf, 3 + MBRcvDataBuf[2]);
	crc2 = (MBRcvDataBuf[MBRcvDataLen - 2] << 8)
			+ MBRcvDataBuf[MBRcvDataLen - 1];
	if (crc != crc2)
		return -1;

	if (Com_Param.mb_var[MBCurrentDev].type == INTEGER) {
		if (MBRcvDataBuf[2] != 0x02) {
			return -1;
		}
		d1 = MBRcvDataBuf[3];
		d2 = MBRcvDataBuf[4];
		INT16U val = (d1 << 8) + d2;

		if (val > Com_Param.mb_var[MBCurrentDev].ulv) {
			Com_Val[MBCurrentDev] = Com_Param.mb_var[MBCurrentDev].ulv;
		} else if (val < Com_Param.mb_var[MBCurrentDev].llv) {
			Com_Val[MBCurrentDev] = Com_Param.mb_var[MBCurrentDev].llv;
		} else {
			Com_Val[MBCurrentDev] = val;
		}
	} else {
		if (MBRcvDataBuf[2] != 0x04) {
			return -1;
		}

		pfdata.byte[0] = MBRcvDataBuf[4];
		pfdata.byte[1] = MBRcvDataBuf[3];
		pfdata.byte[2] = MBRcvDataBuf[6];
		pfdata.byte[3] = MBRcvDataBuf[5];
		float val_upper = Com_Param.mb_var[MBCurrentDev].ulv;
		float val_lower = Com_Param.mb_var[MBCurrentDev].llv;
		if (pfdata.data > val_upper) {
			Com_Val[MBCurrentDev] = val_upper;
		} else if (pfdata.data < val_lower) {
			Com_Val[MBCurrentDev] = val_lower;
		} else {
			Com_Val[MBCurrentDev] = pfdata.data;
		}
	}

	Com_Err[MBCurrentDev] = 0;

	return 0;
}
//=====================================
//modbus ���������
//=====================================
void ProcessModbus(void)
{
	int flag, ret;

	flag = 0;
	while (1) {
		switch (MBState) {
		case 0:		//��������
			if (Com_Param.mb_var[MBCurrentDev].enable == 0) {
				MBState = 3;
			} else {
				if (++Com_Err[MBCurrentDev] > MBMAXERRCNT) {
					Com_Val[MBCurrentDev] = 0;
					Com_Err[MBCurrentDev] = 0;
				}

				SendModbusCmd();
				MBState = 1;
				flag = 1;
			}
			break;
		case 1:		//����Ӧ��
			ret = RcvModbusRsp();
			if (0 == ret) {
				flag = 1;
			} else if (-1 == ret) {
				MBState = 3;
				Com_Param.mb_var[MBCurrentDev].status = ERROR;
			} else {
				MBState = 2;
				Com_Param.mb_var[MBCurrentDev].status = NORMAL;
			}
			break;
		case 2:		//����Ӧ��
			ret = AnalystModbusRsp();
			if (-1 == ret) {
				Com_Param.mb_var[MBCurrentDev].status = ERROR;
			}
			MBState = 3;
			break;
		case 3:		//�л��豸
			++MBCurrentDev;
			if (MBCurrentDev < MODBUS_VARNUM)	//����ɨ����һ���豸
			{
				MBState = 0;
				flag = 1;
			} else						//�ȴ�ֱ��һ��ɨ�����ڽ����ٽ�����һ��ɨ������
			{
				MBCurrentDev = 0;
				MBState = 0xff;
				flag = 1;
			}
			break;
		case 0xff:
			if (MbScanCnt >= MBSCANCNTNUM) {
				MbScanCnt = 0;
				MBState = 0;
				flag = 1;
			} else {
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

///////////////////////////////////////////
