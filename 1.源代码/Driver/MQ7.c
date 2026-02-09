#include "MQ7.h"
#include <math.h>
#include "delay.h"
#include "adc.h"

#define CALPPM 10 //校准环境中PPM值
#define  R0 3.50   //R0是器件在洁净空气中的电阻值，来自于MQ-7灵敏度特性曲线，R0 = RS / pow(CAL_PPM / MQ7_COEF_A, 1 / MQ7_COEF_B);  CAL_PPM=10 

//读取MQ-7的ppm值
//co_ppm = a * pow(Rs/R0, b); 使用校准曲线计算一氧化碳浓度
//a, b是MQ-7传感器模块校准曲线的系数.
float MQ7_readPpm(void) {
	return (float) MQ7_COEF_A * pow(MQ7_readRs() / R0 , MQ7_COEF_B);
}

//传感器电阻(Rs)，可用下式计算:
// Rs\RL = (Vc-VRL) / VRL
// Rs = ((Vc-VRL) / VRL) * RL
float MQ7_readRs(void) {
	float voltage;
	voltage = MQ7_convertVoltage();   //获取电压值
	return ((5.0-voltage)/voltage) * MQ7_LOAD_RES;
}

//读取电压转换值
float MQ7_convertVoltage(void) {
	// ATD conversion
	return (float) (3.3 * (Get_Adc_Average(ADC_Channel_9,10) / 4096.0));  //读取电压值
}

//R0是器件在洁净空气中的电阻值
float MQ7_getR0(void)
{
		return  (MQ7_readRs() / pow(CALPPM / MQ7_COEF_A, 1 / MQ7_COEF_B));
}


