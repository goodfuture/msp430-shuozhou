

#ifndef _AT24C64_H
#define _AT24C64_H

#include "ad.h"
#include "control.h"
#include "modbus.h"

void AT24C64_W(void *src,INT16U des,INT16U len);
void AT24C64_RS(void *des,INT16U src,INT16U len);

//////////////////////////////////////////

//各种参数的存储地址
#define VERSIONCODE_ADDR    0X00
//#define FLOWCOUNT_ADDR      0X10
//#define PROCSTAT_ADDR      	0X20
#define SYSPARAM_ADDR       0X100
#define ADPARAM_ADDR        0X200
#define COMPARAM_ADDR       0X300
#define ADCALIBPARAM_ADDR   0X1000


void Read_VersionCode(char *Code);
void Save_VersionCode(char *Code);

void Default_SysParam(SYS_PARAM *Param);
void Read_SysParam(SYS_PARAM *Param);
void Save_SysParam(SYS_PARAM *Param);

void Default_ADParam(AD_PARAM *Param);
void Read_ADParam(AD_PARAM *Param);
void Save_ADParam(AD_PARAM *Param);

void Default_ComParam(COM_PARAM *Param);
void Read_ComParam(COM_PARAM *Param);
void Save_ComParam(COM_PARAM *Param);

void Fix_ADCalibParam(AD_CALIBPARAM *Param);
void Read_ADCalibParam(AD_CALIBPARAM *Param);
void Save_ADCalibParam(AD_CALIBPARAM *Param);

#endif

