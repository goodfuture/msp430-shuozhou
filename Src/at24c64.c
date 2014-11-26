#include <string.h>
#include <stdio.h>
#include "system.h"
#include "at24c64.h"

#define AT24WR_PROTECT    P3OUT|=BIT0
#define AT24WR_UNPROTECT  P3OUT&=~BIT0

#define I2C_START     UCB0CTL1|=UCTXSTT
#define I2C_TRANSMIT  UCB0CTL1|=UCTR
#define I2C_RECEIVE   UCB0CTL1&=~UCTR
#define I2C_STOP      UCB0CTL1|=UCTXSTP

#define FIRST_STATION

void AT24C64_W(void *src, INT16U des, INT16U len)
{
	volatile INT16U i;
	INT8U *Source;

	Source = (INT8U *) src;
	I2C_TRANSMIT;   //transmit mode
	UCB0I2CSA = 0x50;
	AT24WR_UNPROTECT;
	for (i = 0; i < len; i++) {
		I2C_TRANSMIT;
		I2C_START;
		while ((IFG2 & UCB0TXIFG)==0);
		UCB0TXBUF = (INT8U) (des >> 8);
		while ((IFG2 & UCB0TXIFG)==0);
		UCB0TXBUF = (INT8U) des;
		while ((IFG2 & UCB0TXIFG)==0);
		UCB0TXBUF = *Source++;
		while ((IFG2 & UCB0TXIFG)==0);
		I2C_STOP;
		while (UCB0CTL1 & UCTXSTP)
			;
		des++;
		Delay_N_mS(350);
	}
	AT24WR_PROTECT;
}

void AT24C64_RS(void *des, INT16U src, INT16U len)
{
	INT16U i;
	INT8U *Dest;
	Dest = (INT8U *) des;

	I2C_TRANSMIT;   //transmit mode
	I2C_START;
	while ((IFG2 & UCB0TXIFG)==0);
	UCB0TXBUF = (INT8U) (src >> 8);
	while ((IFG2 & UCB0TXIFG)==0);
	UCB0TXBUF = (INT8U) src;
	while ((IFG2 & UCB0TXIFG)==0);

	I2C_RECEIVE;
	I2C_START;

	while ((IFG2 & UCB0RXIFG)==0);
	*Dest = UCB0RXBUF;

	for (i = 0; i < len; i++) {
		while ((IFG2 & UCB0RXIFG)==0);
		*Dest++ = UCB0RXBUF;
	}
	I2C_STOP;
}

////////////////////////////////

//程序版本号
void Read_VersionCode(char *Code)
{
	AT24C64_RS(Code, VERSIONCODE_ADDR, 6);
	Delay_N_mS(500);
	AT24C64_RS(Code, VERSIONCODE_ADDR, 6);
	Delay_N_mS(500);
}

void Save_VersionCode(char *Code)
{
	char temp[6];
	//写了再读，保证数据正确写入
	while (1) {
		AT24C64_W(Code, VERSIONCODE_ADDR, 6);
		Read_VersionCode(temp);
		if (memcmp(temp, Code, 6) == 0)
			break;
	}
}

//系统参数
void Default_SysParam(SYS_PARAM *Param)
{
	strcpy(Param->pw, "123456");
	strcpy(Param->st, "32");
#ifdef FIRST_STATION
	//strcpy(Param->mn, "12345678901234");
	strcpy(Param->mn, "14060300000202");
#else
	strcpy(Param->mn, "14060300000203");
#endif
	strcpy(Param->sim, "00000000000");
	strcpy(Param->rtd, "300");
	strcpy(Param->pw, "123456");
}

void Read_SysParam(SYS_PARAM *Param)
{
	AT24C64_RS(Param, SYSPARAM_ADDR, sizeof(SYS_PARAM));
	Delay_N_mS(500);
	AT24C64_RS(Param, SYSPARAM_ADDR, sizeof(SYS_PARAM));
	Delay_N_mS(500);
}

void Save_SysParam(SYS_PARAM *Param)
{
	SYS_PARAM temp;
	while (1) {
		AT24C64_W(Param, SYSPARAM_ADDR, sizeof(SYS_PARAM));

		Read_SysParam(&temp);
		if (memcmp(&temp, Param, sizeof(SYS_PARAM)) == 0)
			break;
	}
}

//AD参数
void Default_ADParam(AD_PARAM *Param)
{
	char i;
	for (i = 0; i < 8; ++i) {
		memset(Param[i].id, 0, sizeof(Param[i].id));
		Param[i].enable = 1;
		Param[i].type = 0;
		Param[i].rate = 0;
		Param[i].offset = 0;
#ifdef FIRST_STATION
		if (i == 0) {
			sprintf((char *) Param[i].id, "251010");
			Param[i].highval = 552;
			Param[i].lowval = 0;
		} else if (i == 1) {
			sprintf((char *) Param[i].id, "251020");
			Param[i].highval = 552;
			Param[i].lowval = 0;
		} else if (i == 2) {
			sprintf((char *) Param[i].id, "416010");
			Param[i].highval = 3000;
			Param[i].lowval = 0;
		} else if (i == 3) {
			sprintf((char *) Param[i].id, "416020");
			Param[i].highval = 3000;
			Param[i].lowval = 0;
		} else if (i == 4) {
			sprintf((char *) Param[i].id, "416030");
			Param[i].highval = 3000;
			Param[i].lowval = 0;
		} else if (i == 5) {
			sprintf((char *) Param[i].id, "328010");
			Param[i].highval = 600;
			Param[i].lowval = 0;
		} else if (i == 6) {
			sprintf((char *) Param[i].id, "261010");
			Param[i].highval = 17;
			Param[i].lowval = 0;
		} else if (i == 7) {
			sprintf((char *) Param[i].id, "261020");
			Param[i].highval = 17;
			Param[i].lowval = 0;
		} else {
			Param[i].enable = 0;
			sprintf((char *) Param[i].id, "000000");
			Param[i].highval = 60;
			Param[i].lowval = 0;
		}
#else
		if (i == 0) {
			sprintf((char *) Param[i].id, "251010");
			Param[i].highval = 800;
			Param[i].lowval = 0;
		} else if (i == 1) {
			sprintf((char *) Param[i].id, "251020");
			Param[i].highval = 800;
			Param[i].lowval = 0;
		} else if (i == 2) {
			sprintf((char *) Param[i].id, "416010");
			Param[i].highval = 3000;
			Param[i].lowval = 0;
		} else if (i == 3) {
			sprintf((char *) Param[i].id, "416020");
			Param[i].highval = 3000;
			Param[i].lowval = 0;
		} else if (i == 4) {
			sprintf((char *) Param[i].id, "416030");
			Param[i].highval = 3000;
			Param[i].lowval = 0;
		} else if (i == 5) {
			sprintf((char *) Param[i].id, "328010");
			Param[i].highval = 600;
			Param[i].lowval = 0;
		} else if (i == 6) {
			sprintf((char *) Param[i].id, "261010");
			Param[i].highval = 10;
			Param[i].lowval = 0;
		} else if (i == 7) {
			sprintf((char *) Param[i].id, "261020");
			Param[i].highval = 10;
			Param[i].lowval = 0;
		} else {
			Param[i].enable = 0;
			sprintf((char *) Param[i].id, "000000");
			Param[i].highval = 60;
			Param[i].lowval = 0;
		}
#endif
	}
}

void Read_ADParam(AD_PARAM *Param)
{
	AT24C64_RS(Param, ADPARAM_ADDR, sizeof(AD_PARAM) * 8);
	Delay_N_mS(500);
	AT24C64_RS(Param, ADPARAM_ADDR, sizeof(AD_PARAM) * 8);
	Delay_N_mS(500);
}

void Save_ADParam(AD_PARAM *Param)
{
	AD_PARAM temp[8];
	while (1) {
		AT24C64_W(Param, ADPARAM_ADDR, sizeof(AD_PARAM) * 8);
		Read_ADParam(temp);
		if (memcmp(temp, Param, sizeof(AD_PARAM) * 8) == 0)
			break;
	}
}

//通信参数
void Default_ComParam(COM_PARAM *Param)
{
	char i;

	Param->baud_index = 0;
	for (i = 0; i < MODBUS_VARNUM; ++i) {
		Param->mb_var[i].enable = 1;
		Param->mb_var[i].type = INTEGER;
		memset(Param->mb_var[i].regid, 0, sizeof(Param->mb_var[i].regid));

#ifdef FIRST_STATION
		if (i == 0) {
			sprintf(Param->mb_var[i].regid, "416040");
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].regaddr = 1;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 3000;
		} else if (i == 1) {
			sprintf(Param->mb_var[i].regid, "416050");
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].regaddr = 2;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 900;
		} else if (i == 2) {
			sprintf(Param->mb_var[i].regid, "339010");
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].regaddr = 3;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 4;
		} else if (i == 3) {
			sprintf(Param->mb_var[i].regid, "339020");
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].regaddr = 4;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 4;
		} else if (i == 4) {
			sprintf(Param->mb_var[i].regid, "376010");
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].regaddr = 5;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 4;
		} else {
			sprintf(Param->mb_var[i].regid, "000000");
			Param->mb_var[i].enable = 0;
			Param->mb_var[i].regaddr = 1;
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 900;
		}
#else
		if (i == 0) {
			sprintf(Param->mb_var[i].regid, "416040");
			Param->mb_var[i].devid = 2;
			Param->mb_var[i].regaddr = 1;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 3000;
		} else if (i == 1) {
			sprintf(Param->mb_var[i].regid, "416050");
			Param->mb_var[i].devid = 2;
			Param->mb_var[i].regaddr = 2;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 900;
		} else if (i == 2) {
			sprintf(Param->mb_var[i].regid, "339030");
			Param->mb_var[i].devid = 2;
			Param->mb_var[i].regaddr = 3;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 30;
		} else if (i == 3) {
			sprintf(Param->mb_var[i].regid, "376020");
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].regaddr = 5;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 4;
		} else {
			sprintf(Param->mb_var[i].regid, "000000");
			Param->mb_var[i].enable = 0;
			Param->mb_var[i].regaddr = 1;
			Param->mb_var[i].devid = 1;
			Param->mb_var[i].llv = 0;
			Param->mb_var[i].ulv = 900;
		}
#endif
	}
}

void Read_ComParam(COM_PARAM *Param)
{
	AT24C64_RS(Param, COMPARAM_ADDR, sizeof(COM_PARAM));
	Delay_N_mS(500);
	AT24C64_RS(Param, COMPARAM_ADDR, sizeof(COM_PARAM));
	Delay_N_mS(500);
}

void Save_ComParam(COM_PARAM *Param)
{
	COM_PARAM temp;
	while (1) {
		AT24C64_W(Param, COMPARAM_ADDR, sizeof(COM_PARAM));
		Read_ComParam(&temp);
		if (memcmp(&temp, Param, sizeof(COM_PARAM)) == 0)
			break;
	}
}

//对AD校准参数进行修正
void Fix_ADCalibParam(AD_CALIBPARAM *Param)
{
	char i;

	for (i = 0; i < 8; ++i) {
		if ((Param->k4[i] < 0x100) || (Param->k20[i] > 0x850)) {
			Param->k4[i] = 0x19b;
			Param->k20[i] = 0x7f9;
		}
	}
}

void Read_ADCalibParam(AD_CALIBPARAM *Param)
{
	AT24C64_RS(Param, ADCALIBPARAM_ADDR, sizeof(AD_CALIBPARAM));
	Delay_N_mS(500);
	AT24C64_RS(Param, ADCALIBPARAM_ADDR, sizeof(AD_CALIBPARAM));
	Delay_N_mS(500);
}

void Save_ADCalibParam(AD_CALIBPARAM *Param)
{
	AD_CALIBPARAM temp;
	while (1) {
		AT24C64_W(Param, ADCALIBPARAM_ADDR, sizeof(AD_CALIBPARAM));

		Read_ADCalibParam(&temp);
		if (memcmp(&temp, Param, sizeof(AD_CALIBPARAM)) == 0)
			break;
	}
}
