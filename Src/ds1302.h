

#ifndef _DS1302_H
#define _DS1302_H

#include "system.h"


void v_Set1302(INT8U *System_Time);
void v_Get1302(INT8U *System_Time, T_Times *Tm);

#endif
