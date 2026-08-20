#ifndef DRV_ADS1113_H__
#define DRV_ADS1113_H__

#include "stdint.h"
#define DEF_FABU(a, b)			((a) >= (b) ? (a - b) : (b - a))


uint8_t ADS1113_V_Read(float *u);



#endif

