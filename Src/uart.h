

#ifndef _UART_H
#define _UART_H

#define COM1      0
#define GPRS      0
#define COM2      1
#define COM3      2
#define COM4      3
#define COM5      4

#define RINGBUF_SIZE 200

typedef struct _COM_BUF
{
  unsigned char buf[RINGBUF_SIZE];
  int head;
  int tail;
} COM_BUF;

extern COM_BUF Com_Buf[5];


int PutComBuf(COM_BUF *pbuf, unsigned char *pdata, int datasize);
int GetComBuf(COM_BUF *pbuf, unsigned char *pdata, int datasize);

void Uart_SendData(unsigned char *src,int len,int com);
int Uart_RcvData(unsigned char *dst,int len,int com);
void Uart_ClearRcvBuf(int com);


#endif  //end of _UART_H

