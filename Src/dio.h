

#ifndef _DIO_H
#define _DIO_H

extern INT8U PWR_Stat;

extern INT32U DI_Val;
extern INT16U DO_Val;

INT16U Get_DIVal(void);
void Set_DOVal(INT8U Val);

void ProcDIO(void);

#endif

