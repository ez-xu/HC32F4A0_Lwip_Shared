#include "main.h"

BMS_REG BMSData;

void MP2797_Init(uint8_t mode)
{	
	if(mode) 
	{
		MP2797_Init_Fast();
	}
	else 
	{
		MP2797_Init_Normal();
	}
}

void MP2797_Comm(uint8_t mode)
{
	if(mode)
	{
		MP2797_Comm_Fast();
	}
	else
	{
		MP2797_Comm_Normal();
	}
}
















