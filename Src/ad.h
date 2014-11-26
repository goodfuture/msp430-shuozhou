#ifndef _AD_H
#define _AD_H

#include "system.h"
#include "prodata.h"

typedef struct _AD_PARAM
{
	INT8U enable;
	INT8U id[7];    //污染物编号
	INT8U type;     //AI转换种类4－20mA,0－5V,0－10V，
	float highval;  //上位值
	float lowval;   //下位值
	float rate;     //斜率
	float offset;   //偏置
} AD_PARAM;

typedef struct _AD_CALIBPARAM{
    INT16U k20[8];  //20ma的校准值
    INT16U k4[8];   //4ma的校准值
}AD_CALIBPARAM;


extern AD_CALIBPARAM Ad_CalibParam;
extern AD_PARAM Ad_Param[8];
extern float Ad_RealData[8];
extern MAX_MIN_DATA Ad_MaxMin[8];

void ADParam_Calculate(void);
void ProcADC(void);

////////////////////////////////
float GetO2Val(char Index);

#endif
