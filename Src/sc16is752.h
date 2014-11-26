

#ifndef _SC16IS752_H
#define _SC16IS752_H


void IS752_Init(unsigned long baud,int com);
void IS752_ChangeBaudRate(unsigned long baud,int com);
void IS752_SendData(unsigned char *data,int len,int com);


#endif      //end of _SC16IS752_H
