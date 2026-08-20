#ifndef _MP2797_FUNC_FAST_H
#define _MP2797_FUNC_FAST_H
#include "main.h"

#define RECORD_LEN	200	

typedef struct 
{
	uint16_t vol[RECORD_LEN];
	uint16_t cur[RECORD_LEN];
}Record_st;

void MP2797_Init_Fast(void);
void MP2797_Comm_Fast(void);
void Store_Record(void);

#endif
