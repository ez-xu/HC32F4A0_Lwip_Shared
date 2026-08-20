#ifndef _MP2797_FUNC_H
#define _MP2797_FUNC_H
#include "main.h"

#define VOL_CALCULATE(x)		(((float)x/(float)32767*3.3f)/(float)27*(float)14027)
#define VOL_CALCULATE_JY2(x)	(((float)x/(float)32767*3.3f)/(float)27*(float)7027)
#define RES_CALCULATE(x)		((double)x/(double)27000*((double)3138000+(double)27000))

#define FACTOR_K(x)				((float)x/1000)
#define FACTOR_B(x)				(((float)x-30000)/10)

void MP2797_Init_Normal(void);
void MP2797_Comm_Normal(void);
uint32_t Res_AvrFilter(uint32_t* res_buff,uint8_t len);

#endif
