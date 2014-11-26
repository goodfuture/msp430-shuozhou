#include "system.h"
#include "flash.h"

#define CONTINUOUS_MREAD      0X03
#define BUFFER1_READ          0XD1
#define BUFFER2_READ          0XD3
#define BUFFER1_WRITE         0x84
#define BUFFER2_WRITE         0x87
#define MEMORY_WRITE_THBUFF1  0X82
#define MEMORY_WRITE_THBUFF2  0X85
#define BUFFER1_TO_MEMORY     0x83
#define BUFFER2_TO_MEMORY     0x86
#define MEMORY_TO_BUFFER1     0x53
#define MEMORY_TO_BUFFER2     0x55

//使能flash之前，需要先关闭中断，否则读取出来的数据可能存在问题

#define FLASH_ENABLE  {Stop_WDT();DISABLEINT();P4OUT &=~BIT1;}/*P5OUT &=~BIT2*/
//推出flash之后，要开中断
#define FLASH_DISABLE {P4OUT |=BIT1;ENABLEINT();Start_WDT();}/*P5OUT |=BIT2*/
#define FLASH_RST     P4OUT &=~BIT0;P4OUT |=BIT0
#define FLASH_PROTECT P5OUT &=~BIT7
#define FLASH_UNPROTECT P5OUT |=BIT7

#define FlashSendPrepare()  {U1TXBUF = 0;while(!(TXEPT&U1TCTL));}

/////////////////////////////////////////////
void FlashSendByte(unsigned char x)
{
	while (!(IFG2 & UTXIFG1)) {
	};
	U1TXBUF = x;
}

unsigned char FlashReadByte(unsigned char snd)
{
	unsigned char v;
	while (!(IFG2 & UTXIFG1)) {
	};
	U1TXBUF = snd;
	while (!(IFG2 & URXIFG1)) {
	};
	v = U1RXBUF;
	return v;
}
/*
 *********************************************************************************************************
 *                               FLASH MEMORY READ
 *
 * Description: This function is used to read the main memory of the flash
 *
 * Arguments  : addr:  memory address where to store the data
 *              array: pointer to the stored data
 *              len:   the length of the data to be stored
 *
 * Returns    : none
 *
 * write by   : Wen Jianjun
 *
 * data       : 0722 2008
 *
 *********************************************************************************************************
 */
void FlashMemoryRead(INT32U addr, void *array, INT16U len)
{
	char i;
	char *Dest;

	Dest = (char *) array;
	FlashSendPrepare()
	; //very important for msp430
	  //_BIC_SR(GIE);
	FLASH_ENABLE
	;
	/*send command*/
	FlashSendByte(CONTINUOUS_MREAD);

	/*send address 3 bytes*/
	for (i = 0; i < 3; i++) {
		FlashSendByte((char) ((addr >> (8 * (2 - i))) & 0x000000ff));
	}
	/*read a dummy byte*/
	Delay_N_mS(20);
	FlashReadByte(0);

	/*read data from flash*/
	while (len--) {
		*Dest++ = FlashReadByte(0);
	}
	//_BIS_SR(GIE);
	Delay_N_mS(20);
	FLASH_DISABLE
	;
}
/*
 *********************************************************************************************************
 *                               FLASH BUFFER READ
 *
 * Description: This function is used to read the data from the buffers of the flash
 *
 * Arguments  : addr:  memory address where to read the data
 *              array: pointer to the stored data
 *              len:   the length of the data to read
 *              buff:  indicate that which buffer to read from(the value should be one
 *                     of these:FLASHBUFFER1,FLASHBUFFER2)
 *
 * Returns    : none
 *
 * write by   : Wen Jianjun
 *
 * data       : 0722 2008
 *
 *********************************************************************************************************
 */
void FlashBufferRead(INT32U addr, void *array, INT16U len, INT8U buff)
{
	char i;
	char *Dest;

	Dest = (char *) array;
	FlashSendPrepare()
	; //very important for msp430
	  //_BIC_SR(GIE);
	FLASH_ENABLE
	;
	/*send command*/
	//while(!(IFG2&UTXIFG1));
	if (buff == FLASHBUFFER1) {
		FlashSendByte(BUFFER1_READ);
		//U1TXBUF =BUFFER1_READ;
	} else {
		FlashSendByte(BUFFER2_READ);
		//U1TXBUF =BUFFER2_READ;
	}

	/*send address 3 bytes*/
	for (i = 0; i < 3; i++) {
		FlashSendByte((char) ((addr >> (8 * (2 - i))) & 0x000000ff));
	}
	/*send one don't care byte*/
	//while(!(IFG2&UTXIFG1));
	//U1TXBUF =0;
	/*receive a dummy byte*/
	FlashReadByte(0);

	/*read data from flash*/
	while (len--) {
		*Dest++ = FlashReadByte(0);
	}
	//_BIS_SR(GIE);
	FLASH_DISABLE
	;
}
/*
 *********************************************************************************************************
 *                               FLASH BUFFER WRITE
 *
 * Description: This function is used to write the buffers of the flash
 *
 * Arguments  : addr:  memory address where to store the data
 *              array: pointer to the stored data
 *              len:   the length of the data to be stored
 *              buff:  indicate that which buffer to write to (the value should be one
 *                     of these:FLASHBUFFER1,FLASHBUFFER2)
 *
 * Returns    : none
 *
 * write by   : Wen Jianjun
 *
 * data       : 0722 2008
 *
 *********************************************************************************************************
 */
void FlashBufferWrite(INT32U addr, void *array, INT16U len, INT8U buff)
{
	char i;
	char *Dest;

	FLASH_UNPROTECT;
	Dest = (char *) array;
	FlashSendPrepare()
	; //very important for msp430
	FLASH_ENABLE
	;
	//_BIC_SR(GIE);
	/*send command*/
	//while(!(IFG2&UTXIFG1));
	if (buff == FLASHBUFFER1) {
		FlashSendByte(BUFFER1_WRITE);
		//U1TXBUF =BUFFER1_WRITE;
	} else {
		FlashSendByte(BUFFER2_WRITE);
		//U1TXBUF =BUFFER2_WRITE;
	}
	/*send address 3 bytes*/
	for (i = 0; i < 3; i++) {
		FlashSendByte((char) ((addr >> (8 * (2 - i))) & 0x000000ff));
	}
	/*write data to flash buffer*/
	while (len--) {
		FlashSendByte(*Dest++);
	}
	while (!(TXEPT & U1TCTL))
		;
	//Delay_N_mS(20);
	//_BIS_SR(GIE);
	FLASH_DISABLE
	;
	FLASH_PROTECT;
}
/*
 *********************************************************************************************************
 *                               FLASH MEMORY WRITE
 *
 * Description: This function is used to program the memory of the flash through the buffers
 *
 * Arguments  : addr:  memory address where to store the data
 *              array: pointer to the stored data
 *              len:   the length of the data to be stored
 *              buff:  indicate that which buffer to be used (the value should be one
 *                     of these:FLASHBUFFER1,FLASHBUFFER2)
 *
 * Returns    : none
 *
 * write by   : Wen Jianjun
 *
 * data       : 0722 2008
 *
 *********************************************************************************************************
 */
void FlashMemoryWrite(INT32U addr, void *array, INT16U len, INT8U buff)
{
	char i;
	char *Dest;

	FLASH_UNPROTECT;
	Dest = (char *) array;

	FlashSendPrepare()
	;

	FLASH_ENABLE
	;
	//_BIC_SR(GIE);
	/*send command*/
	if (buff == FLASHBUFFER1) {
		//FlashSendByte(MEMORY_WRITE_THBUFF1);
		U1TXBUF = MEMORY_WRITE_THBUFF1;
	} else {
		//FlashSendByte(MEMORY_WRITE_THBUFF2);
		U1TXBUF = MEMORY_WRITE_THBUFF2;
	}
	/*send address 3 bytes*/
	for (i = 0; i < 3; i++) {
		FlashSendByte((char) ((addr >> (8 * (2 - i))) & 0x000000ff));
	}
	/*write data to flash memory*/
	Delay_N_mS(20);
	while (len--) {
		FlashSendByte(*Dest++);
	}
	while (!(TXEPT & U1TCTL))
		;
	//Delay_N_mS(10);
	//_BIS_SR(GIE);
	FLASH_DISABLE
	;
	FLASH_PROTECT;
	Delay_N_mS(2500);
}

void FlashMemorytoBuffer(INT32U addr, INT8U buff)
{
	char i;

	FlashSendPrepare()
	; //very important for msp430
	FLASH_ENABLE
	;
	//_BIC_SR(GIE);
	if (buff == FLASHBUFFER1) {
		FlashSendByte(MEMORY_TO_BUFFER1);
		//U1TXBUF =MEMORY_TO_BUFFER1;
	} else {
		FlashSendByte(MEMORY_TO_BUFFER2);
		//U1TXBUF =MEMORY_TO_BUFFER2;
	}
	for (i = 0; i < 3; i++) {
		FlashSendByte((char) ((addr >> (8 * (2 - i))) & 0x000000ff));
	}
	while (!(TXEPT & U1TCTL))
		;
	//_BIS_SR(GIE);
	FLASH_DISABLE
	;
	Delay_N_mS(500);
}

void FlashBuffertoMemory(INT32U addr, INT8U buff)
{
	char i;

	FLASH_UNPROTECT;
	FlashSendPrepare()
	;
	FLASH_ENABLE
	;
	//_BIC_SR(GIE);
	if (buff == FLASHBUFFER1) {
		FlashSendByte(BUFFER1_TO_MEMORY);
		//U1TXBUF =BUFFER1_TO_MEMORY;
	} else {
		FlashSendByte(BUFFER2_TO_MEMORY);
		//U1TXBUF =BUFFER2_TO_MEMORY;
	}
	for (i = 0; i < 3; i++) {
		FlashSendByte((char) ((addr >> (8 * (2 - i))) & 0x000000ff));
	}
	while (!(TXEPT & U1TCTL))
		;
	//_BIS_SR(GIE);
	FLASH_DISABLE
	;
	FLASH_PROTECT;
	Delay_N_mS(2500);
}

//void FlashClearMemory(char *addr,int len,char buff)
void FlashClearMemory(INT32U addr, INT16U len, INT8U buff)
{
	char i;
	//char *Dest;

	//Dest  =(char *)array;
	FlashSendPrepare()
	; //very important for msp430
	FLASH_UNPROTECT;
	FLASH_ENABLE
	;
	/*send command*/
	//while((IFG2&UTXIFG1)==0);
	if (buff == FLASHBUFFER1) {
		FlashSendByte(MEMORY_WRITE_THBUFF1);
	} else {
		FlashSendByte(MEMORY_WRITE_THBUFF2);
	}
	/*send address 3 bytes*/
	for (i = 0; i < 3; i++) {
		FlashSendByte((char) ((addr >> (8 * (2 - i))) & 0x000000ff));
	}
	/*write data to flash memory*/
	Delay_N_mS(20);
	while (len--) {
		FlashSendByte(0);
	}
	while (!(TXEPT & U1TCTL))
		;
	//Delay_N_mS(10);
	FLASH_DISABLE
	;
	FLASH_PROTECT;
	Delay_N_mS(2500);
}

#if 0
#include <string.h>

void FlashTest(void)
{
	//char addr[3] = {0,100,0};
	INT32U addr = 0x00010010;
	//INT32U addr = 0;
	char str[32] = "today is thusday";
	char tmp[32] = "\0";

	while(1)
	{
		FlashMemoryWrite(addr,str,strlen(str),FLASHBUFFER1);
		//Delay_N_mS(20000);
		FlashMemoryRead(addr,tmp,strlen(str));
		Delay_N_mS(20000);
		char str1[] = "i hate this way";
		FlashMemoryWrite(addr,str1,strlen(str1),FLASHBUFFER1);
		Delay_N_mS(20000);
		FlashMemoryRead(addr,tmp,strlen(str1));
		Delay_N_mS(20000);
		Delay_N_mS(20000);
	}
}
#endif

