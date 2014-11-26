

#ifndef _FLASH_H
#define _FLASH_H


//flash地址生成，pa页地址，ba页内地址
#define FLASHADDR(pa,ba)  ((((u32)(pa))<<11)|((u32)ba))

#define FLASHBUFFER1       0X00
#define FLASHBUFFER2       0X01

#define FLASH_INIT()     P4OUT &=~BIT0;P4OUT |=BIT0

void FlashMemoryRead(INT32U addr,void *array,INT16U len);
void FlashBufferRead(INT32U addr,void *array,INT16U len,INT8U buff);
void FlashBufferWrite(INT32U addr,void *array,INT16U len,INT8U buff);
void FlashMemoryWrite(INT32U addr,void *array,INT16U len,INT8U buff);
void FlashMemorytoBuffer(INT32U addr,INT8U buff);
void FlashBuffertoMemory(INT32U addr,INT8U buff);
void FlashClearMemory(INT32U addr,INT16U len,INT8U buff);


#endif   //end of _FLASH_H

