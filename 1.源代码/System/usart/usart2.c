#include <stdio.h>
#include "usart2.h"
#include <stdbool.h>
  
unsigned char Usart2RecBuf[USART2_RXBUFF_SIZE];//串口2接收数据缓存
unsigned int  Rx2Counter = 0;   //串口2收到数据标志位
bool rev_start  = 0;     //接收开始标志
bool rev_stop   = 0;     //接收停止标志

void USART2_Init(u32 baud)   
 {  
      USART_InitTypeDef USART_InitStructure;  
      NVIC_InitTypeDef NVIC_InitStructure;   
      GPIO_InitTypeDef GPIO_InitStructure;    

      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE); 
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);  
  
      // Configure USART2 Rx (PA.3) as input floating    
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;  
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
      GPIO_Init(GPIOA, &GPIO_InitStructure);  
  
      // Configure USART2 Tx (PA.2) as alternate function push-pull  
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;  
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  
      GPIO_Init(GPIOA, &GPIO_InitStructure);  
  
      USART_InitStructure.USART_BaudRate = baud;  
      USART_InitStructure.USART_WordLength = USART_WordLength_8b;  
      USART_InitStructure.USART_StopBits = USART_StopBits_1;  
      USART_InitStructure.USART_Parity = USART_Parity_No;  
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
      USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  
  
  
      // Configure USART2   
      USART_Init(USART2, &USART_InitStructure);
      // Enable USART2 Receive interrupts 
      USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  
      // Enable the USART2   
      USART_Cmd(USART2, ENABLE);
  
      //Configure the NVIC Preemption Priority Bits     
      NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
  
      // Enable the USART2 Interrupt   
      NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;   
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
      NVIC_Init(&NVIC_InitStructure);       
  } 
 
void USART2_Sned_Char(u8 temp)        
 {  
     USART_SendData(USART2,(u8)temp);      
    while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);  
 }

void Uart2_SendStr(char*SendBuf)//串口2打印数据
{
	while(*SendBuf)
	{
	  USART2_Sned_Char(*SendBuf); 
		SendBuf++;
	}
}

void uart2_send(unsigned char *bufs,unsigned char len)
{
	  if(len != 0xFF)
		{
				while (len--)
				{
						USART2_Sned_Char(*bufs); 
						bufs ++;
				}
		}
		else//如果字长等于或超过255，不按用户写入字长发送
    {
        for (; *bufs != 0;	bufs++)USART2_Sned_Char(*bufs); 	//把字符逐个发送出去
    }
}

void USART2_IRQHandler(void)                
{
	   u8 ch;
	
     if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  
     {   
			   ch = USART_ReceiveData(USART2);//接收模块的数据
			   if ((ch == 0x2C)&&(rev_stop==0))  //如果收到0x2C，便开始接收
					{
							rev_start = 1;
					}
					if (rev_start == 1)  //标志位为1，开始接收
					{
							Usart2RecBuf[Rx2Counter] = ch;  //字符存到数组中
						  Rx2Counter ++;
							if (Rx2Counter >= 9) 	         //一帧数据接收完成
							{
									rev_start = 0;
									rev_stop  = 1;
									Rx2Counter = 0;
							}
					}
     }

     if(USART_GetFlagStatus(USART2,USART_FLAG_ORE) == SET)
     {
         USART_ClearFlag(USART2,USART_FLAG_ORE);
     }
     USART_ClearITPendingBit(USART2, USART_IT_RXNE);
}



