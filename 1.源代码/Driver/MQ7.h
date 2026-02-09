#ifndef MQ7_H
#define MQ7_H

#include "sys.h"   

#define MQ7_COEF_A 98.322
#define MQ7_COEF_B -1.458

/* 
datasheet provides typical load resistance RL to be ~1 kOhm
*/
#define MQ7_LOAD_RES 1.0

float MQ7_readPpm(void);
float MQ7_readRs(void);
float MQ7_getR0(void);
float MQ7_convertVoltage(void);

#endif
