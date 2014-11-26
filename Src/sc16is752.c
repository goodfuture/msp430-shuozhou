
#include "system.h"
#include "sc16is752.h"
#include "uart.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CRYSTALFREQUENCY	14745600L

//for 16is752
#define RHR752 			0x00
#define THR752 			0x00
#define IER752 			0x01
#define FCR752 			0x02
#define IIR752 			0x02
#define LCR752 			0x03
#define MCR752 			0x04
#define LSR752 			0x05
#define MSR752 			0x06
#define SPR752 			0x07
#define TCR752 			0x06
#define TLR752 			0x07
#define TXLVL752 		0x08
#define RXLVL752 		0x09
#define IODIR752 		0x0A
#define IOSTATE752 		0x0B
#define IOINTENA752 	0x0C
#define IOCONTROL752	0x0E
#define EFCR752 		0x0F
#define DLL752 			0x00
#define DLH752 			0x01
#define EFR752 			0x02
#define XON1752 		0x04
#define XON2752 		0x05
#define XOFF1752 		0x06
#define XOFF2752 		0x07

#define NO_PARITY   	0x00
#define EVEN_PARITY 	0x18
#define ODD_PARITY  	0x08

#define RCVRDY  		0x01
#define OVRERR  		0x02
#define PRTYERR 		0x04
#define FRMERR  		0x08
#define BRKERR  		0x10
#define XMTRDY  		0x20
#define XMTRSR  		0x40
#define TIMEOUT			0x80

#define DTR 			0x01
#define RTS 			0x02
#define MC_INT			0x08

#define CTS 			0x10
#define DSR 			0x20

#define RX_INT  		0x01
#define TX_INT			0x02

#define NO_INT			0x01
#define URS_ID			0x06
#define RX_ID   		0x04
#define FDT_ID			0x0c
#define TBRE_ID			0x02
#define HSR_ID			0x00
#define RX_MASK 		0x0f

/*******************************************************************
 752的片选, 1为使能,0为禁止
 *******************************************************************/
void IS752En(unsigned char chip, unsigned char value)
{
	if (chip == 0) {
		if (value == 1)
			P7OUT &= ~BIT1;
		else if (value == 0)
			P7OUT |= BIT1;
	} else if (chip == 1) {
		if (value == 1)
			P4OUT &= ~BIT2;
		else if (value == 0)
			P4OUT |= BIT2;
	}
}

/*******************************************************************
 复位752
 *******************************************************************/
void IS752Reset(unsigned char chip)
{
	if (chip == 0) {
		P8OUT &= ~BIT3;
		Delay_10_uS();
		P8OUT |= BIT3;
		Delay_10_uS();
	} else if (chip == 1) {
		P7OUT &= ~BIT6;
		Delay_10_uS();
		P7OUT |= BIT6;
		Delay_10_uS();
	}
}

/*******************************************************************
 写752的寄存器
 *******************************************************************/
void WriteRegister_IS752(unsigned char com, unsigned char address,
		unsigned char data)
{
	unsigned char chip;
	unsigned char channel;

	switch (com) {
	case COM2:
		chip = 0;
		channel = 0;
		break;
	case COM3:
		chip = 0;
		channel = 1;
		break;
	case COM4:
		chip = 1;
		channel = 0;
		break;
	case COM5:
		chip = 1;
		channel = 1;
		break;
	default:
		return;
	}

	IS752En(chip, 1);

	address <<= 3;
	channel <<= 1;
	address |= channel;

	while ((U1TCTL & TXEPT)!=0X01);	//发地址
	IFG2 &= ~(UTXIFG1 + URXIFG1);
	U1TXBUF = address;
	while (!(IFG2 & UTXIFG1))
		;			//发数据
	//while((U1TCTL&TXEPT)!=0X01);
	U1TXBUF = data;
	while ((U1TCTL & TXEPT)!=0X01);

	IS752En(chip, 0);
}

/*******************************************************************
 读752的寄存器
 *******************************************************************/
unsigned char ReadRegister_IS752(unsigned char com, unsigned char address)
{
	unsigned char chip;
	unsigned char channel;
	unsigned char data = 0;

	switch (com) {
	case COM2:
		chip = 0;
		channel = 0;
		break;
	case COM3:
		chip = 0;
		channel = 1;
		break;
	case COM4:
		chip = 1;
		channel = 0;
		break;
	case COM5:
		chip = 1;
		channel = 1;
		break;
	default:
		return 0;
	}

	IS752En(chip, 1);

	address <<= 3;
	channel <<= 1;
	address |= channel;
	address |= 0x80;

	//send cmd
	while ((U1TCTL & TXEPT)!=0X01);
	IFG2 &= ~(UTXIFG1 + URXIFG1);
	U1TXBUF = address;

	//read
	while (!(IFG2 & UTXIFG1))
		;
	//while((U1TCTL&TXEPT)!=0X01);
	U1TXBUF = 0;
	while ((U1TCTL & TXEPT)!=0X01);
	data = U1RXBUF;

	IS752En(chip, 0);

	return data;
}

void IS752_SendData(unsigned char *data, int len, int com)
{
	int num, cnt;

	for (num = 0; num < len; ++num) {
		for (cnt = 0; cnt < 10000; ++cnt) //防止占用太长时间
				{
			if ((ReadRegister_IS752(com, TXLVL752) & 0x7f) != 0) {
				WriteRegister_IS752(com, THR752, data[num]);
				break;
			}
		}
		if (cnt >= 10000)	//发送失败
			break;
	}
}

void IS752_Init(unsigned long baud, int com)
{
	unsigned long int divisor;

	divisor = CRYSTALFREQUENCY / 16L / baud;

	//IS752Reset();
	WriteRegister_IS752(com, LCR752, 0xBF);
	WriteRegister_IS752(com, EFR752, 0x10);
	WriteRegister_IS752(com, LCR752, 0x80);
	WriteRegister_IS752(com, DLL752, (unsigned char) (divisor & 0xff));
	WriteRegister_IS752(com, DLH752, (unsigned char) ((divisor >> 8) & 0xff));
	WriteRegister_IS752(com, LCR752, 0x03);

	WriteRegister_IS752(com, IER752, 0x01);
	WriteRegister_IS752(com, FCR752, 0x07);
	Delay_N_mS(200); 	//wait for the end of fifo reset
	WriteRegister_IS752(com, MCR752, 0x04);
	//WriteRegister_IS752(com, TCR752, 0x00);
	//WriteRegister_IS752(com, TLR752, 0x00);
	WriteRegister_IS752(com, IODIR752, 0xFF);
	WriteRegister_IS752(com, IOSTATE752, 0xFF);
	WriteRegister_IS752(com, IOCONTROL752, 0x00);
	WriteRegister_IS752(com, IOINTENA752, 0x00);
	WriteRegister_IS752(com, EFCR752, 0x10);
}

void IS752_ChangeBaudRate(unsigned long baud, int com)
{
	unsigned long int divisor;

	divisor = CRYSTALFREQUENCY / 16L / baud;

	WriteRegister_IS752(com, LCR752, 0x80);
	WriteRegister_IS752(com, DLL752, (unsigned char) (divisor & 0xff));
	WriteRegister_IS752(com, DLH752, (unsigned char) ((divisor >> 8) & 0xff));
	WriteRegister_IS752(com, LCR752, 0x03);
}

/*******************************************
 external interrupt function
 used for IS752(extended uart)
 *******************************************/
#pragma vector=PORT2_VECTOR
__interrupt void EXTERNALISR(void)
{
	unsigned char data;
	int com, num;

	if (P2IFG & BIT1) {
		P2IFG &= ~BIT1;

		for (com = COM2; com <= COM3; ++com) {
			num = 0;
			while ((ReadRegister_IS752(com, RXLVL752) & 0x7f) != 0) {
				data = ReadRegister_IS752(com, RHR752);
				PutComBuf(&Com_Buf[com], &data, 1);
				if (++num > 4)		//防止长时间占用中断
					break;
			}
		}
	}
	if (P2IFG & BIT2) {
		P2IFG &= ~BIT2;

		for (com = COM4; com <= COM5; ++com) {
			num = 0;
			while ((ReadRegister_IS752(com, RXLVL752) & 0x7f) != 0) {
				data = ReadRegister_IS752(com, RHR752);
				PutComBuf(&Com_Buf[com], &data, 1);
				if (++num > 4)		//防止长时间占用中断
					break;
			}
		}
	}
}

