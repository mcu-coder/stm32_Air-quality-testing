/************************************************************************************
* 
* Description:128*64点阵的OLED显示屏驱动文件SD1306驱动IIC通信方式显示屏
*
* Others: none;
*
* Function List:
*	1. void I2C_Configuration(void) -- 配置CPU的硬件I2C
* 2. void I2C_WriteByte(uint8_t addr,uint8_t data) -- 向寄存器地址写一个byte的数据
* 3. void WriteCmd(unsigned char I2C_Command) -- 写命令
* 4. void WriteDat(unsigned char I2C_Data) -- 写数据
* 5. void OLED_Init(void) -- OLED屏初始化
* 6. void OLED_SetPos(unsigned char x, unsigned char y) -- 设置起始点坐标
* 7. void OLED_Fill(unsigned char fill_Data) -- 全屏填充
* 8. void OLED_CLS(void) -- 清屏
* 9. void OLED_ON(void) -- 唤醒
* 10. void OLED_OFF(void) -- 睡眠
* 11. void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize) -- 显示字符串(字体大小有6*8和8*16两种)
* 12. void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N) -- 显示中文(中文需要先取模，然后放到codetab.h中)
* 13. void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]) -- BMP图片
*
* History: none;
*
*************************************************************************************/

#include "OLED_I2C.h"
#include "delay.h"
#include "codetab.h"

/* 定义I2C总线连接的GPIO端口, 用户只需要修改下面4行代码即可任意改变SCL和SDA的引脚 */

#define RCC_I2C_PORT 	RCC_APB2Periph_GPIOB		/* GPIO端口时钟 */

#define PORT_I2C_SCL	GPIOB			/* GPIO端口 */
#define PIN_I2C_SCL		GPIO_Pin_6		/* GPIO引脚 */

#define PORT_I2C_SDA	GPIOB			/* GPIO端口 */
#define PIN_I2C_SDA		GPIO_Pin_7		/* GPIO引脚 */

#define I2C_SCL_PIN		GPIO_Pin_6			/* 连接到SCL时钟线的GPIO */
#define I2C_SDA_PIN		GPIO_Pin_7			/* 连接到SDA数据线的GPIO */

/* 定义读写SCL和SDA的宏 */
#define I2C_SCL_1()  PORT_I2C_SCL->BSRR = I2C_SCL_PIN				/* SCL = 1 */
#define I2C_SCL_0()  PORT_I2C_SCL->BRR = I2C_SCL_PIN				/* SCL = 0 */

#define I2C_SDA_1()  PORT_I2C_SDA->BSRR = I2C_SDA_PIN				/* SDA = 1 */
#define I2C_SDA_0()  PORT_I2C_SDA->BRR = I2C_SDA_PIN				/* SDA = 0 */

#define I2C_SDA_READ()  ((PORT_I2C_SDA->IDR & I2C_SDA_PIN) != 0)	/* 读SDA口线状态 */
#define I2C_SCL_READ()  ((PORT_I2C_SCL->IDR & I2C_SCL_PIN) != 0)	/* 读SCL口线状态 */

/*
*********************************************************************************************************
*	函 数 名: bsp_InitI2C
*	功能说明: 配置I2C总线的GPIO，采用模拟IO的方式实现
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitI2C_2(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_I2C_PORT, ENABLE);	/* 打开GPIO时钟 */

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;	/* 开漏输出模式 */
	
	GPIO_InitStructure.GPIO_Pin = PIN_I2C_SCL;
	GPIO_Init(PORT_I2C_SCL, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PIN_I2C_SDA;
	GPIO_Init(PORT_I2C_SDA, &GPIO_InitStructure);

	/* 给一个停止信号, 复位I2C总线上的所有设备到待机模式 */
	i2c_Stop_2();
}

/*
*********************************************************************************************************
*	函 数 名: i2c_Start
*	功能说明: CPU发起I2C总线启动信号
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void i2c_Start_2(void)
{
	/* 当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号 */
	I2C_SDA_1();
	I2C_SCL_1();
	delay_us(4);
	I2C_SDA_0();
	delay_us(4);
	I2C_SCL_0();
}

/*
*********************************************************************************************************
*	函 数 名: i2c_Start
*	功能说明: CPU发起I2C总线停止信号
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void i2c_Stop_2(void)
{
	/* 当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号 */
	I2C_SDA_0();
	I2C_SCL_1();
	delay_us(4);
	I2C_SDA_1();
	delay_us(4);
}

/*
*********************************************************************************************************
*	函 数 名: i2c_SendByte
*	功能说明: CPU向I2C总线设备发送8bit数据
*	形    参:  _ucByte ： 等待发送的字节
*	返 回 值: 无
*********************************************************************************************************
*/
void i2c_SendByte_2(uint8_t _ucByte)
{
	uint8_t i;

	/* 先发送字节的高位bit7 */
	for (i = 0; i < 8; i++)
	{
		if (_ucByte & 0x80)
		{
			I2C_SDA_1();
		}
		else
		{
			I2C_SDA_0();
		}
		delay_us(2);
		I2C_SCL_1();
		delay_us(2);
		I2C_SCL_0();
		_ucByte <<= 1;	/* 左移一个bit */
		delay_us(2);
	}
}

/*
*********************************************************************************************************
*	函 数 名: i2c_ReadByte
*	功能说明: CPU从I2C总线设备读取8bit数据
*	形    参:  无
*	返 回 值: 读到的数据
*********************************************************************************************************
*/
uint8_t i2c_ReadByte_2(void)
{
	uint8_t i;
	uint8_t value;

	/* 读到第1个bit为数据的bit7 */
	value = 0;
	for (i = 0; i < 8; i++)
	{
		value <<= 1;
		I2C_SCL_1();//SCL在高电平期间，数据必须保持稳定
		delay_us(2);
		if (I2C_SDA_READ())
		{
			value++;
		}
		I2C_SCL_0();
		delay_us(1);
	}
	return value;
}

/*
*********************************************************************************************************
*	函 数 名: i2c_WaitAck
*	功能说明: CPU产生一个时钟，并读取器件的ACK应答信号
*	形    参:  无
*	返 回 值: 返回0表示正确应答，1表示无器件响应
*********************************************************************************************************
*/
uint8_t i2c_WaitAck_2(void)
{
	uint8_t tempTime;
	
	I2C_SDA_1();	/* CPU释放SDA总线 */
	delay_us(1);
	I2C_SCL_1();	/* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
	delay_us(1);
	while(I2C_SDA_READ())
	{
		tempTime++;
		if(tempTime>250)
		{
			i2c_Stop_2();
			return 1;
		}	 
	}
	I2C_SCL_0();

	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: i2c_Ack
*	功能说明: CPU产生一个ACK信号
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void i2c_Ack_2(void)
{
	I2C_SDA_0();	/* CPU驱动SDA = 0 */
	delay_us(5);
	I2C_SCL_1();	/* CPU产生1个时钟 */
	delay_us(5);
	I2C_SCL_0();
	delay_us(5);
	I2C_SDA_1();	/* CPU释放SDA总线 */
}

/*
*********************************************************************************************************
*	函 数 名: i2c_NAck
*	功能说明: CPU产生1个NACK信号
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void i2c_NAck_2(void)
{
	I2C_SDA_1();	/* CPU驱动SDA = 1 */
	delay_us(5);
	I2C_SCL_1();	/* CPU产生1个时钟 */
	delay_us(5);
	I2C_SCL_0();
	delay_us(5);
}

void I2C_Configuration(void)
{
	bsp_InitI2C_2();
}

void I2C_WriteByte(uint8_t addr,uint8_t data)
{
		/* 第1步：发起I2C总线启动信号 */
		i2c_Start_2();	

		/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
		i2c_SendByte_2(OLED_ADDRESS | I2C_WR);	/* 此处是写指令 */

		/* 第3步：发送ACK */
		i2c_WaitAck_2();
		
		/* 第4步：发送字节地址 */
		i2c_SendByte_2(addr);
		
		i2c_WaitAck_2();
		
		/* 第5步：开始写入数据 */
		i2c_SendByte_2(data);
		
		/* 第6步：发送ACK */
		i2c_WaitAck_2();
		
		/* 发送I2C总线停止信号 */
		i2c_Stop_2();
}

void WriteCmd(unsigned char I2C_Command)//写命令
{
	I2C_WriteByte(0x00, I2C_Command);
}

void WriteDat(unsigned char I2C_Data)//写数据
{
	I2C_WriteByte(0x40, I2C_Data);
}

void OLED_Init(void)
{
	delay_ms(100); //这里的延时很重要
	
	WriteCmd(0xAE); //display off
	WriteCmd(0x20);	//Set Memory Addressing Mode	
	WriteCmd(0x10);	//00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	WriteCmd(0xb0);	//Set Page Start Address for Page Addressing Mode,0-7
	WriteCmd(0xc8);	//Set COM Output Scan Direction
	WriteCmd(0x00); //---set low column address
	WriteCmd(0x10); //---set high column address
	WriteCmd(0x40); //--set start line address
	WriteCmd(0x81); //--set contrast control register
	WriteCmd(0xff); //亮度调节 0x00~0xff
	WriteCmd(0xa1); //--set segment re-map 0 to 127
	WriteCmd(0xa6); //--set normal display
	WriteCmd(0xa8); //--set multiplex ratio(1 to 64)
	WriteCmd(0x3F); //
	WriteCmd(0xa4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	WriteCmd(0xd3); //-set display offset
	WriteCmd(0x00); //-not offset
	WriteCmd(0xd5); //--set display clock divide ratio/oscillator frequency
	WriteCmd(0xf0); //--set divide ratio
	WriteCmd(0xd9); //--set pre-charge period
	WriteCmd(0x22); //
	WriteCmd(0xda); //--set com pins hardware configuration
	WriteCmd(0x12);
	WriteCmd(0xdb); //--set vcomh
	WriteCmd(0x20); //0x20,0.77xVcc
	WriteCmd(0x8d); //--set DC-DC enable
	WriteCmd(0x14); //
	WriteCmd(0xaf); //--turn on oled panel
}

void OLED_SetPos(unsigned char x, unsigned char y) //设置起始点坐标
{ 
	WriteCmd(0xb0+y);
	WriteCmd(((x&0xf0)>>4)|0x10);
	WriteCmd((x&0x0f)|0x01);
}

void OLED_Fill(unsigned char fill_Data)//全屏填充
{
	unsigned char m,n;
	for(m=0;m<8;m++)
	{
		WriteCmd(0xb0+m);		//page0-page1
		WriteCmd(0x00);		//low column start address
		WriteCmd(0x10);		//high column start address
		for(n=0;n<128;n++)
			{
				WriteDat(fill_Data);
			}
	}
}

void OLED_Fill_Row(unsigned char fill_Data,unsigned char y)//填充指定行，
{
	unsigned char m,n;
	for(m=0;m<2;m++)
	{
		WriteCmd(0xb0+y);		//page0-page1
		WriteCmd(0x00);		//low column start address
		WriteCmd(0x10);		//high column start address
		for(n=0;n<128;n++)
			{
				WriteDat(fill_Data);
			}
	}
}

void OLED_CLS(u8 mode)//清屏
{
	if(mode==0)OLED_Fill(0x00);  //清屏黑色
	else       OLED_Fill(0xFF);  //清屏白色
}

//--------------------------------------------------------------
// Prototype      : void OLED_ON(void)
// Calls          : 
// Parameters     : none
// Description    : 将OLED从休眠中唤醒
//--------------------------------------------------------------
void OLED_ON(void)
{
	WriteCmd(0X8D);  //设置电荷泵
	WriteCmd(0X14);  //开启电荷泵
	WriteCmd(0XAF);  //OLED唤醒
}

//--------------------------------------------------------------
// Prototype      : void OLED_OFF(void)
// Calls          : 
// Parameters     : none
// Description    : 让OLED休眠 -- 休眠模式下,OLED功耗不到10uA
//--------------------------------------------------------------
void OLED_OFF(void)
{
	WriteCmd(0X8D);  //设置电荷泵
	WriteCmd(0X10);  //关闭电荷泵
	WriteCmd(0XAE);  //OLED休眠
}

//--------------------------------------------------------------
//函数名： void OLED_DrawPoint(u8 x,u8 y)
//功能描述：绘制一个点（x,y）
//参数：真实坐标值(x,y),x的范围0～127，y的范围0～64
//返回：无
//--------------------------------------------------------------
void OLED_DrawPoint(u8 x,u8 y)
{
    u8 data1;  //data1当前点的数据 
	
		data1 = 0x01<<(y%8); 	
		WriteCmd(0xb0+(y/8));
		WriteCmd(((x&0xf0)>>4)|0x10);
		WriteCmd((x&0x0f)|0x00);
		WriteDat(data1); 
}


//--------------------------------------------------------------
//函数名： void OLED_DrawHorizLine(u8 y,u8 x0,u8 x1)
//功能描述：绘制水平线条
//参数：y为水平线位置，x0为起始位置，x1为结束位置
//返回：无
//--------------------------------------------------------------
void OLED_DrawHorizLine(u8 y,u8 x0,u8 x1)
{
	u8 x = 0;
	
	for (x = x0;x <= x1;x++)
	{
		OLED_DrawPoint(x,y);
	}
}

//--------------------------------------------------------------
//函数名： void OLED_DrawVerticaLine(u8 x,u8 y0,u8 y1)
//功能描述：绘制水平线条
//参数：x为垂直线位置，y0为起始位置，y1为结束位置
//返回：无
//--------------------------------------------------------------
void OLED_DrawVerticaLine(u8 x,u8 y0,u8 y1)
{
	  u8 i = 0;
	
	  for(i=y0;i<y1;i++)
	  {
				WriteCmd(0xb0+i);
				WriteCmd(((x&0xf0)>>4)|0x10);
				WriteCmd((x&0x0f)|0x00);
				WriteDat(0xFF);
		}
}

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~7
//chr:显示的字符		 
//TextSize:字符大小(1:6*8 ; 2:8*16)
//mode:1,反白显示;0,正常显示		
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char TextSize,u8 mode)
{      	
		unsigned char c=0,i=0;	
	
		c=chr-' ';//得到偏移后的值	
		if(TextSize == 2)
		{
				if(x>120){x=0;y++;}
			  OLED_SetPos(x,y);
				for(i=0;i<8;i++)
					if(mode==1)WriteDat(~(F8X16[c*16+i]));else WriteDat(F8X16[c*16+i]);
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
					if(mode==1)WriteDat(~(F8X16[c*16+i+8]));else WriteDat(F8X16[c*16+i+8]);
		}
		else 
		{	
				if(x>126){x=0;y++;}
				OLED_SetPos(x,y);
				for(i=0;i<6;i++)
					if(mode==1)WriteDat(~(F6x8[c][i])); else WriteDat(F6x8[c][i]);
		}
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
// Calls          : 
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); ch[] -- 要显示的字符串; TextSize -- 字符大小(1:6*8 ; 2:8*16)
// Description    : 显示codetab.h中的ASCII字符,有6*8和8*16可选择
//--------------------------------------------------------------
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize,u8 mode)
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)
	{
		case 1:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 126)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<6;i++)
				if(mode==1)WriteDat(~(F6x8[c][i]));
				else 	WriteDat(F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 120)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<8;i++)
				if(mode==1)WriteDat(~(F8X16[c*16+i]));
				else 	WriteDat(F8X16[c*16+i]);
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
				if(mode==1)WriteDat(~(F8X16[c*16+i+8]));
				else	WriteDat(F8X16[c*16+i+8]);
				x += 8;
				j++;
			}
		}break;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
// Calls          : 
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); N:汉字在codetab.h中的索引
// Description    : 显示codetab.h中的汉字,16*16点阵
//mode:1,反白显示;0,正常显示	
//--------------------------------------------------------------
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N,u8 mode)
{
	unsigned char wm=0;
	unsigned int  adder=32*N;
	OLED_SetPos(x , y);
	for(wm = 0;wm < 16;wm++)
	{
		if(mode==1)WriteDat(~(F16x16[adder]));else WriteDat(F16x16[adder]);
		adder += 1;
	}
	OLED_SetPos(x,y + 1);
	for(wm = 0;wm < 16;wm++)
	{
		if(mode==1)WriteDat(~(F16x16[adder]));else WriteDat(F16x16[adder]);
		adder += 1;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
// Calls          : 
// Parameters     : x0,y0 -- 起始点坐标(x0:0~127, y0:0~7); x1,y1 -- 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
// Description    : 显示BMP位图
//--------------------------------------------------------------
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[])
{
	unsigned int j=0;
	unsigned char x,y;

  if(y1%8==0)
		y = y1/8;
  else
		y = y1/8 + 1;
	for(y=y0;y<y1;y++)
	{
		OLED_SetPos(x0,y);
    for(x=x0;x<x1;x++)
		{
			WriteDat(BMP[j++]);
		}
	}
}

void OLED_ShowCentigrade(unsigned char x, unsigned char y,u8 mode)//显示℃
{
	unsigned char wm=0;
	unsigned char BUF[]={
	0x10,0x28,0x10,0xC0,0x20,0x10,0x10,0x10,0x20,0x70,0x00,0x00,0x00,0x00,0x00,0x07,
	0x08,0x10,0x10,0x10,0x10,0x08,0x04,0x00,/*"℃"*/
	};
	OLED_SetPos(x , y);
	for(wm = 0;wm < 12;wm++)
	{
		 
		 if(mode==1)WriteDat(~(BUF[wm]));else WriteDat(BUF[wm]);
	}
	OLED_SetPos(x,y + 1);
	for(wm = 0;wm < 12;wm++)
	{
		 if(mode==1)WriteDat(~(BUF[wm+12]));else WriteDat(BUF[wm+12]);
	}
}

void OLED_ShowStrPm(unsigned char x, unsigned char y, unsigned char TextSize,u8 mode)
{
	    unsigned char i = 0;
	    unsigned char num=0;
	    u8 str_buf[]={
				0x08,0xF8,0xF8,0x08,0x08,0x08,0xF8,0xF0,0x20,0x3F,0x3F,0x21,0x01,0x01,0x01,0x00,/*"P",0*/
				0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x20,0x3F,0x3F,0x20,0x3F,0x3F,0x20,0x3F,/*"m",1*/
			};
	
			while(num < 2)
			{
				if(x > 120)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<8;i++)
				if(mode==1)WriteDat(~(str_buf[num*16+i]));
				else 	WriteDat(str_buf[num*16+i]);
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
				if(mode==1)WriteDat(~(str_buf[num*16+i+8]));
				else	WriteDat(str_buf[num*16+i+8]);
				x += 8;
				num++;
			}
}

