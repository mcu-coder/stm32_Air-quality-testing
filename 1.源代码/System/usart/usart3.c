#include <stdio.h>
#include "usart3.h"
#include <stdbool.h>
  
unsigned char Usart3RecBuf[USART3_RXBUFF_SIZE];//串口3接收数据缓存
unsigned int Rx3Counter = 0;   //串口3收到数据标志位
bool B_RX_OK=0;
bool rx3_rev_start = 0;

void USART3_Init(u32 baud)   
 {  
      USART_InitTypeDef USART_InitStructure;  
      NVIC_InitTypeDef NVIC_InitStructure;   
      GPIO_InitTypeDef GPIO_InitStructure;    

      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE); 
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);  
  
      // Configure USART3 Rx (PB.11) as input floating    
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;  
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
      GPIO_Init(GPIOB, &GPIO_InitStructure);  
  
      // Configure USART3 Tx (PB.10) as alternate function push-pull  
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;  
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  
      GPIO_Init(GPIOB, &GPIO_InitStructure);  
  
      USART_InitStructure.USART_BaudRate = baud;  
      USART_InitStructure.USART_WordLength = USART_WordLength_8b;  
      USART_InitStructure.USART_StopBits = USART_StopBits_1;  
      USART_InitStructure.USART_Parity = USART_Parity_No;  
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
      USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  
  
  
      // Configure USART3   
      USART_Init(USART3, &USART_InitStructure);
      // Enable USART3 Receive interrupts 
      USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);  
      // Enable the USART3   
      USART_Cmd(USART3, ENABLE);
  
      //Configure the NVIC Preemption Priority Bits     
      NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
  
      // Enable the USART3 Interrupt   
      NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;   
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
      NVIC_Init(&NVIC_InitStructure);       
  } 
 
void USART3_Sned_Char(u8 temp)        
 {  
     USART_SendData(USART3,(u8)temp);      
    while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);  
 }

void Uart3_SendStr(char*SendBuf)//串口3打印数据
{
	while(*SendBuf)
	{
	  USART3_Sned_Char(*SendBuf); 
		SendBuf++;
	}
}

void uart3_send(unsigned char *bufs,unsigned char len)
{
	  if(len != 0xFF)
		{
				while (len--)
				{
						USART3_Sned_Char(*bufs); 
						bufs ++;
				}
		}
		else//如果字长等于或超过255，不按用户写入字长发送
    {
        for (; *bufs != 0;	bufs++)USART3_Sned_Char(*bufs); 	//把字符逐个发送出去
    }
}

void USART3_IRQHandler(void)                
{
	   u8 ch;
	
     if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  
     {   
				 ch = USART_ReceiveData(USART3);//接收模块的数据
			   if ((ch == 0XA5)&&(B_RX_OK==0))  //如果收到0xA5，便开始接收
					{
							rx3_rev_start = 1;
					}
					if (rx3_rev_start == 1)  //标志位为1，开始接收
					{
							Usart3RecBuf[Rx3Counter] = ch;  //字符存到数组中
						  Rx3Counter ++;
							if (Rx3Counter >= 4) 	         //一帧数据接收完成
							{
									rx3_rev_start = 0;
									B_RX_OK  = 1;
									Rx3Counter = 0;
							}
					}
     }

     if(USART_GetFlagStatus(USART3,USART_FLAG_ORE) == SET)
     {
         USART_ClearFlag(USART3,USART_FLAG_ORE);
     }
     USART_ClearITPendingBit(USART3, USART_IT_RXNE);
}


