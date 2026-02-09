#include "sys.h"
#include "delay.h"
#include "adc.h"
#include "gpio.h"
#include "OLED_I2C.h"
#include "timer.h"
#include "MQ7.h"
#include "dht11.h"
#include "esp8266.h"
#include "usart2.h"
#include "usart3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define STM32_RX2_BUF       Usart2RecBuf          //串口2接收缓存区
#define STM32_Rx2Counter    Rx2Counter           //串口2接收字节计数
#define STM32_RX2BUFF_SIZE  USART2_RXBUFF_SIZE   //串口2接收缓存大小

#define STM32_RX3_BUF       Usart3RecBuf        //串口3接收缓存区
#define STM32_Rx3Counter    Rx3Counter          //串口3接收字节计数
#define STM32_RX3BUFF_SIZE  USART3_RXBUFF_SIZE  //串口3接收缓存大小

#define  RATIO  1.0		   //校准系数，选择范围0.1~1.0 （传感器一般不需要校准，选择1.0即可）
u16 PM25_Value = 0;     		//PM2.5
u16 PM25_Value_max = 200;  //PM2.5报警值
u8 pmBuf[5];               //pm2.5数据缓存数组
u8 ch2oBuf[8];            //甲醛数据缓存数组

char display[16];
unsigned char setn=0;                //记录设置键按下的次数
unsigned char temperature=0;         //温度变量
unsigned char humidity=0;            //湿度变量
unsigned int  CH2O_mgvalue = 0;      //甲醛变量
unsigned char setTempValue=35;        //温度设置值
unsigned char setHumiValue=75;        //湿度设置值
unsigned int setCH2OValue=10;        //甲醛设置值
unsigned char i=0;

unsigned int  co_ppm = 0;                //co值
unsigned int setCoMaxValue=200;          //CO上限

bool shuaxin  = 1;              //刷新数据标志
bool shanshuo = 0;              //液晶闪烁标志
bool sendFlag = 1;              //串口发送数据标志

void displayOriginalInt(void)  //显示原始页面
{
//	  OLED_Fill_Row(0xff,0);    //反白显示
//	  OLED_Fill_Row(0xff,1);    //反白显示

	  for(i=0;i<2;i++)OLED_ShowCN(i*16+0,0,i+18,0);//显示中文：温度
	  for(i=0;i<2;i++)OLED_ShowCN(i*16+71,0,i+20,0);//显示中文：湿度
	  for(i=0;i<4;i++)OLED_ShowCN(i*16+0,6,i+24,0);//显示中文：一氧化碳
		for(i=0;i<2;i++)OLED_ShowCN(i*16+0,4,i+4,0);//显示中文：甲醛
	  
		OLED_ShowStrPm(0, 2, 2,0);
	  OLED_ShowStr(16, 2, "2.5:", 2,0);
		OLED_ShowChar(64,6,':',2,0);
		OLED_ShowChar(32,4,':',2,0);
	  OLED_ShowCentigrade(48, 0,0);
	  OLED_ShowChar(119,0,'%',2,0);
}

void Get_PM2_5(void)  //获取PM2.5值
{
    char i = 0;

	  STM32_Rx3Counter = 0;
    if(B_RX_OK == 1)              //串口数据接收完成
    {
        for(i = 0; i < 4; i++)
        {
					  pmBuf[i] = STM32_RX3_BUF[i];
        }
				/* 校验数据是否接收正确，校验方法：判断前面3个字节累加和取低7位是否和最后第4个字节相等 */
				if(((pmBuf[0]+pmBuf[1]+pmBuf[2])&0x7F) == pmBuf[3])  
				{
						PM25_Value = (unsigned int)((pmBuf[1]*128) + pmBuf[2]) * RATIO;   //计算PM2.5(ug/m3) = (pmBuf[1]*128) + pmBuf[2] ；
				}
        B_RX_OK = 0;
    }
}

void Get_CH2O(void)  //获取甲醛
{
    char i = 0;
    u16 sum1=0;
	  u8  sum2=0;
	
	  if(rev_stop == 1)                   //数据接收完成了
	  {
				for(i = 0; i < 8; i++)
				{
						sum1 += STM32_RX2_BUF[i];   //累加和
				}
				sum2=(unsigned char)sum1;
				if(sum2 == STM32_RX2_BUF[8])  //校验数据是否正确: 校验和(B8)=unit_8(B0+B1+B2+B3+B4+B5+B6+B7)
				{
						CH2O_mgvalue = (STM32_RX2_BUF[4]*256 + STM32_RX2_BUF[5]); //计算甲醛值 
				}
				rev_stop = 0;
	  }
}

void Get_MQ7_PPM(void)   //读取一氧化碳值
{
	 co_ppm = MQ7_readPpm();         //读取一氧化碳ppm值
	 if(co_ppm >= 1000)co_ppm = 1000; // 最大1000ppm
	
	 if((co_ppm>=setCoMaxValue) && shanshuo)   //co超过上限，液晶闪烁显示
	 {
		   OLED_ShowStr(71, 6, "    ppm", 2,0);
	 }
	 else
		 
	 {
		   if(co_ppm >= 1000)sprintf(display,"%04dppm",co_ppm); 
		   else  sprintf(display," %03dppm",co_ppm); 
			 OLED_ShowStr(71, 6, (u8 *)display, 2,0);       //显示一氧化碳
	 }
}

void displaySetValue(void)  //显示设置的值
{
		if(setn == 1)
		{
			  /*显示温度上限*/
				OLED_ShowChar(52,4,setTempValue/10+'0',2,0);
				OLED_ShowChar(60,4,setTempValue%10+'0',2,0);
				OLED_ShowCentigrade(70, 4,0);
		}
		if(setn == 2)
		{
			  /*显示湿度上限*/
				OLED_ShowChar(52,4,setHumiValue/10+'0',2,0);
				OLED_ShowChar(60,4,setHumiValue%10+'0',2,0);
				OLED_ShowChar(68,4,'%',2,0);
			  OLED_ShowChar(76,4,' ',2,0);
		}
		if(setn == 3)
		{
			  /*显示甲醛上限*/
				sprintf(display,"%4.2fmg/m3",(float)setCH2OValue/100);
			  OLED_ShowStr(28, 4, (u8 *)display, 2,0);
		}
		if(setn == 4)
		{
			  /*显示PM2.5上限*/
				sprintf(display,"%03dug/m3 ",PM25_Value_max);
				OLED_ShowStr(28, 4, (u8 *)display, 2,0);
		}
		if(setn == 5)
		{
			  /*显示CO上限*/
				sprintf(display," %dppm  ",setCoMaxValue); 
				OLED_ShowStr(27, 4, (u8 *)display, 2,0);
		}
}

void keyscan(void)   //按键扫描
{ 
	 if(KEY1 == 0) //加键按下
	 {
			delay_ms(80);
		  if(KEY2 == 0)
			{
					if(setTempValue<99 && setn==1)setTempValue++;                  //温度上限加
					if(setHumiValue<99 && setn==2)setHumiValue++;                 //湿度上限加
				  if(setCH2OValue<999 && setn==3)setCH2OValue++;                //甲醛上限加
					if(PM25_Value_max<999 && setn==4)PM25_Value_max++;	          //PM2.5上限加
				  if(setCoMaxValue<1000 && setn==5)setCoMaxValue++;                    //一氧化碳上限加
				  displaySetValue();                                           //显示设置的值
			}
	 }
	 if(KEY2 == 0) //减键按下
	 {
			delay_ms(80);
		  if(KEY3 == 0)
			{
					if(setTempValue>0 && setn==1)setTempValue--;                //温度上限减
					if(setHumiValue>0 && setn==2)setHumiValue--;                //湿度上限减
				  if(setCH2OValue>0 && setn==3)setCH2OValue--;                //甲醛上限减
					if(PM25_Value_max>0 && setn==4)PM25_Value_max--;	          //pm2.5上限减
				  if(setCoMaxValue>0 && setn==5)setCoMaxValue--;                         //一氧化碳上限减
				  displaySetValue();                                            //显示设置的值
			}
	 }
}

void UsartSendReceiveData(void)                       //串口发送和接收数据，用于和手机APP通信
{
		unsigned char *dataPtr = NULL;
		char *str1=0,i;
	  int  setValue=0;
	  char setvalue[5]={0};
	  char SEND_BUF[200];            //发送数据缓存
		char TEMP_BUF[100];           //临时缓存数组
	
	  dataPtr = ESP8266_GetIPD(0);   //接收数据
		if(dataPtr != NULL)           //手机端下发了数据
		{   
				
				if(strstr((char *)dataPtr,"co_max:")!=NULL)    //接收到设置CO上限的指令
				{
						str1 = strstr((char *)dataPtr,"co_max:");
					  
					  while(*str1 < '0' || *str1 > '9')    //判断是不是0到9有效数字
						{
								str1 = str1 + 1;
								delay_ms(10);
						}
						i = 0;
						while(*str1 >= '0' && *str1 <= '9')        //判断是不是0到9有效数字
						{
								setvalue[i] = *str1;
								i ++; str1 ++;
								if(*str1 == ',')break;            //换行符，直接退出while循环
								delay_ms(10);
						}
						setvalue[i] = '\0';            //加上结尾符
						setValue = atoi(setvalue);
						if(setValue>=0 && setValue<=1000)
						{
							  setCoMaxValue=setValue;   //设置CO上限
								displaySetValue();         //显示设置的值
						}
				}
				
				ESP8266_Clear();									//清空缓存
		}
		if(sendFlag==1)    //800ms上传一次数据
		{
			  sendFlag = 0;		
			   
				memset(SEND_BUF,0,sizeof(SEND_BUF));   			//清空缓冲区
			  memset(TEMP_BUF,0,sizeof(TEMP_BUF));   			//清空缓冲区
			  if(temperature>=setTempValue)strcat(SEND_BUF,"temp_warn,");  //发送温度超标指令，手机端接收到后，温度值会显示红色
			  if(humidity>=setHumiValue)strcat(SEND_BUF,"humi_warn,");        //发送湿度超标指令，手机端接收到后，湿度值会显示红色
			  if(PM25_Value>=PM25_Value_max)strcat(SEND_BUF,"pm2.5_warn,");           //发送PM2.5超标指令，手机端接收到后，PM2.5值会显示红色
			  if((CH2O_mgvalue/10)>=setCH2OValue)strcat(SEND_BUF,"ch2o_warn,");       //发送甲醛超标指令，手机端接收到后，甲醛值会显示红色
			  if(co_ppm>=setCoMaxValue)strcat(SEND_BUF,"co_warn,");                   //发送CO超标指令，手机端接收到后，CO值会显示红色
				sprintf(TEMP_BUF,"$temp:%d#,$humi:%d#,$ch2o:%.3f#,$pm2.5:%d#,$co:%d#",temperature,humidity,(float)CH2O_mgvalue/1000,PM25_Value,co_ppm);//发送的数据装载：温度，湿度，甲醛，PM2.5，co
			  strcat(SEND_BUF,TEMP_BUF);
			  ESP8266_SendData((u8 *)SEND_BUF, strlen(SEND_BUF));   //ESP8266模块发送数据到手机端
			  ESP8266_Clear();                                          //清除接收数据的缓存
		}
}

int main(void)
{
	  u16 timeCount=200;
	  bool delay_600ms=1;
	
		delay_init();	           //延时函数初始化	 
    NVIC_Configuration();	   //中断优先级配置
		KEY_GPIO_Init();        //按键引脚初始化 
	  delay_ms(300); 
	  I2C_Configuration();     //IIC初始化
	  OLED_Init();             //OLED液晶初始化
	  OLED_CLS(0);              //清屏
	  KEY_GPIO_Init();        //按键引脚初始化    
	  OLED_ShowStr(0, 2, "   loading...   ", 2,0);//显示加载中
		while(DHT11_Init())
			{
					OLED_ShowStr(0, 2, "  DHT11 Init!  ", 2,0);//显示DHT11初始化！
					delay_ms(500);
			}
    ESP8266_Init();       //ESP8266初始化	
	  delay_ms(1000);
	  Adc_Init();          //ADC初始化
	  
		OLED_CLS(0);              //清屏
		USART2_Init(9600);       //串口2初始化
		USART3_Init(9600);       //串口3初始化
		displayOriginalInt();   //显示原始界面
	  TIM3_Init(99,719);   //定时器初始化，定时1ms
		//Tout = ((arr+1)*(psc+1))/Tclk ; 
		//Tclk:定时器输入频率(单位MHZ)
		//Tout:定时器溢出时间(单位us)
		while(1)
		{ 
			   keyscan();  //按键扫描
			   
			   if(setn == 0)     //不在设置状态下
				 {
					   timeCount ++;
					   if(timeCount >= 180)
						 {
								timeCount = 0;
							  DHT11_Read_Data(&temperature,&humidity);   //读取温湿度
						 }
					   
						 if(shuaxin == 1)        //大概300ms刷新一下
						 { 
								 shuaxin = 0;
								 
							   delay_600ms = !delay_600ms;
							   
							   if(temperature>=setTempValue && shanshuo)    //温度超过上限，液晶闪烁显示
							   {
									   OLED_ShowChar(32,0,' ',2,0);
										 OLED_ShowChar(40,0,' ',2,0);
								 } 
								 else
								 {
										 OLED_ShowChar(32,0,temperature/10+'0',2,0);    //显示温度
										 OLED_ShowChar(40,0,temperature%10+'0',2,0);
								 }
							   
							   if(humidity>=setHumiValue && shanshuo)        //湿度超过上限，液晶闪烁显示
							   {
									   OLED_ShowChar(103,0,' ',2,0);
										 OLED_ShowChar(111,0,' ',2,0);
								 } 
								 else
								 {
										 OLED_ShowChar(103,0,humidity/10+'0',2,0);      //显示湿度
										 OLED_ShowChar(111,0,humidity%10+'0',2,0);
								 }
								 
								 Get_MQ7_PPM();   //读取一氧化碳值
								 Get_CH2O();      //读取甲醛
								 
							   if((CH2O_mgvalue/10)>=setCH2OValue && shanshuo)   //甲醛超过上限，液晶闪烁显示
							   {
									   OLED_ShowStr(46, 4, "     mg/m3", 2,0);
								 } 
								 else
								 {
										sprintf(display,"%.3fmg/m3",(float)CH2O_mgvalue/1000); 
									  OLED_ShowStr(46, 4, (u8 *)display, 2,0);       //显示甲醛
								 }
							   if(delay_600ms)
								 {
										 Get_PM2_5();            //读取PM2.5
										 if(PM25_Value>999)PM25_Value=999;
								 }
								 
								 if(PM25_Value>=PM25_Value_max && shanshuo)    //pm2.5超过上限，液晶闪烁显示
							   {
									   OLED_ShowStr(56, 2, "   ug/m3", 2,0);
								 } 
								 else
								 {
										 sprintf(display,"%03dug/m3",PM25_Value);
										 OLED_ShowStr(56, 2, (u8 *)display, 2,0);    //显示PM2.5
								 }
								 
								 if(temperature>=setTempValue || humidity>=setHumiValue || (CH2O_mgvalue/10)>=setCH2OValue || PM25_Value>=PM25_Value_max || co_ppm>=setCoMaxValue)
								 {BEEP = 1; RELAY=1;}   //超出上限，蜂鸣器提醒。开启风扇
								 else
								 {BEEP = 0; RELAY=0;}   //关闭风扇和蜂鸣器
						 }
				 }
				 UsartSendReceiveData();   //串口发送接收数据，用于和手机APP通信
			   delay_ms(10);
		}
}

void TIM3_IRQHandler(void)//定时器3中断服务程序，用于记录时间
{ 
	  static u16 timeCount1 = 0;
	  static u16 timeCount2 = 0;
	
		if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{ 
				TIM_ClearITPendingBit(TIM3, TIM_IT_Update); //清除中断标志位  

			  timeCount1 ++;
			  timeCount2 ++;
			   
			  if(timeCount2 >= 800)  //800ms发送数据标志置1
				{
						timeCount2 = 0;
					  sendFlag = 1;
				}
	  }
}

