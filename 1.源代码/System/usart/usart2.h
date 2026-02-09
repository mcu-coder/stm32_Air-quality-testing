#ifndef __usart2_H
#define __usart2_H	 
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include <stdbool.h>

#define USART2_RXBUFF_SIZE   54 

extern unsigned int  Rx2Counter;          //外部声明，其他文件可以调用该变量
extern unsigned char Usart2RecBuf[USART2_RXBUFF_SIZE]; //外部声明，其他文件可以调用该变量
extern bool rev_start  ;     //接收开始标志
extern bool rev_stop   ;     //接收停止标志

void USART2_Init(u32 baud);
void USART2_Sned_Char(u8 temp);
void Uart2_SendStr(char*SendBuf);
void uart2_send(unsigned char *bufs,unsigned char len);

#endif


