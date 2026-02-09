#ifndef __DHT11_H
#define __DHT11_H 
#include "sys.h"   

//如果想要修改引脚，只需修改下面的宏
#define DHT11_GPIO_PIN      GPIO_Pin_11
#define DHT11_GPIO_PORT     GPIOA

#define DHT11_OUT_0   GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)//IO为低电平
#define DHT11_OUT_1   GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)//IO为高电平
#define READ_DHT11_IO GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN)//读取IO电平

u8 DHT11_Init(void);
u8 DHT11_Read_Data(u8 *Tdata,u8 *RHdata);

#endif















