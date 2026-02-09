#include "dht11.h"
#include "delay.h"  

/*******************************************************************************
函数名：DHT11_IO_OUT
功能：配置IO输出状态
输入：
输出：
返回值：
*******************************************************************************/
void DHT11_IO_OUT(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure); 
}

/*******************************************************************************
函数名：DHT11_IO_IN
功能：配置IO输入状态
输入：
输出：
返回值：
*******************************************************************************/
void DHT11_IO_IN(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//上拉输入
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure); 
}

/*******************************************************************************
函数名DHT11_Init
功能：初始化DHT11
输入：
输出：
返回值：初始化成功为0，不成功为1
备注：
*******************************************************************************/
u8 DHT11_Init(void)
{
	  unsigned char wait=0;
	
	  DHT11_IO_OUT();   //输出模式
	  DHT11_OUT_0;      //拉低
	  delay_ms(20);    //拉低至少18ms
	  DHT11_OUT_1;     //拉高
	  delay_us(50);    //拉高20-40us
	  DHT11_IO_IN();   //输入模式
	  if(!READ_DHT11_IO)//判断低电平是否来了
		{
				while(!READ_DHT11_IO && wait++<80){delay_us(1);}//等待40-50us的低电平结束
				
				if(wait >= 80)return 1;
				else wait = 0;
					
				while(READ_DHT11_IO && wait++<80){delay_us(1);}//等待40-50us的高电平结束
				
				if(wait >= 80)return 1;
				else return 0;
		}
		else
		{
				return 1;  //没有返回响应信号，直接返回1
		}
}

/*******************************************************************************
函数名：DHT11_ReadByte
功能：从DHT11读一个字节
输入：
输出：
返回值：读取到的字节
备注：
*******************************************************************************/
unsigned char DHT11_ReadByte(void)
{
		unsigned char i;
	  unsigned char wait= 0;
	  unsigned char bit = 0;
    unsigned char dat = 0;
	
	  for (i=0; i<8; i++)//循环8次，读一个字节
		{
			  wait = 0;
				while(!READ_DHT11_IO && wait++<80){delay_us(1);}//等待12-14us的低电平结束
				wait = 0;
				delay_us(40);  //延时40us
				bit = 0;       //数字0
				if(READ_DHT11_IO)bit = 1;       //数字1
        while(READ_DHT11_IO && wait++<150){delay_us(1);}//等待116-118us的高电平结束
				if(wait >= 150)break;   //高电平超时
				dat <<= 1;   
				dat |= bit;  
		}
		return dat;
}

/*******************************************************************************
函数名：DHT11_Read_Data
功能：从DHT11读取温湿度
输入：
输出：
返回值：0读取成功。1读取失败
备注：
*******************************************************************************/
u8 DHT11_Read_Data(u8 *Tdata,u8 *RHdata)
{
	  unsigned char i;
	  unsigned char BUF[5];
	
		if(DHT11_Init()==0)
		{
				for (i=0; i<5; i++)//循环5次，读5个字节
				{
						BUF[i] = DHT11_ReadByte();
				}
				if((BUF[0]+BUF[1]+BUF[2]+BUF[3])==BUF[4])//校验
				{
					*RHdata=BUF[0];//湿度值
					*Tdata =BUF[2];//温度值
				}
		}
		else
		{
				return 1;	//读取失败
		}
		return 0;	   //读取成功
}
