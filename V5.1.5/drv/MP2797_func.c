#include "main.h"    
#include "drv/drv_gpio.h"
#include "lib/lib_algorithm.h"

void MP2797_Init_Normal(void)
{
	uint8_t Reset=0,DataRx[2];
	
	BMSData.ChipSystemError = 1;
	do
	{
		if(Reset++ > 5)
		{
			Reset = 0;
			GPIO_ResetPins(I2C_WAKEUP_PORT, I2C_WAKEUP_PIN);
			vTaskDelay(pdMS_TO_TICKS(100));
			GPIO_SetPins(I2C_WAKEUP_PORT, I2C_WAKEUP_PIN);
		}
		I2C_Initialize();
		I2C_Write16B(REG_COMM_CONFIG, 0x04, 1, USE_CRC, 1);		
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	while(I2C_Read16B(REG_COMM_CONFIG, &DataRx[0], &DataRx[1], USE_CRC, 1));
	BMSData.ChipSystemError = 0;
    I2C_Write16B(REG_HRSCAN0, 0x3F, 0x0B, USE_CRC, 1);  
	I2C_Write16B(REG_HRSCAN2, 0xE7, 0x01, USE_CRC, 1); //GPIO1~3 NTC1~4
	I2C_Write16B(REG_GPIO_CONFIG, 0x33, 0x03, USE_CRC, 1);  
	I2C_Write16B(REG_NTC_CONFIG, 0x80, 0x00, USE_CRC, 1);  
    I2C_Write16B(REG_REG_CONFIG, 0x00, 0x00, USE_CRC, 1);  	
//	I2C_Write16B(REG_CC_CONFIG, 0x03, 0x00, USE_CRC, 1);
	I2C_Write16B(REG_CC_CONFIG, 0x03, 0x41, USE_CRC, 1);
	I2C_Write16B(REG_INT0_EN, 0x00, 0x10, USE_CRC, 1);
}

#include "lib_filter.h"
lib_filter_queue_st queue_pcs_vol;
lib_filter_queue_st queue_shunt_cur;
void MP2797_Comm_Normal(void)
{
	uint8_t i,Ret;
	uint16_t u16temp;
	double tempf;
	
	I2C_Read16B(REG_ADC_STS, (uint8_t*)&BMSData.ADC_STS, ((uint8_t*)&BMSData.ADC_STS)+1, USE_CRC, 1);
	if(I2C_Read16B(REG_ADC_CTRL, (uint8_t*)&BMSData.ADC_CTRL, ((uint8_t*)&BMSData.ADC_CTRL)+1, USE_CRC, 1) == 0)
	{
		if(BMSData.VscanCnt++ > 40)
		{
			BMSData.VscanCnt=0;
			I2C_Write16B(REG_ADC_CTRL, 0x00, 0x00, USE_CRC, 1);
		}
		if(BMSData.ADC_CTRL&0x04)
		{
			I2C_Write16B(REG_BAL_CTRL, 0x00, 0x00, USE_CRC, 1);
			I2C_Write16B(REG_SFT_GO, 0x00, 0x00, USE_CRC, 1);
			I2C_Write16B(REG_ADC_CTRL, 0x00, 0x00, USE_CRC, 1);
		}
		if((!(BMSData.ADC_CTRL&0x01)) && (!(BMSData.ADC_STS&0x300)))
		{
			I2C_Write16B(REG_ADC_CTRL, 0x01, 0x00, USE_CRC, 0);
		}
		if(BMSData.ADC_CTRL&0x02)
		{
			BMSData.VscanCnt=0;
			BMSData.VScanTime = SysParaVoltiage.heartBeat;
			I2C_Write16B(REG_ADC_CTRL, 0x00, 0x00, USE_CRC, 0);	
			for(i=0; i<3; i++){
				Ret = I2C_Read16B(REG_RD_VGPIO1-i, (uint8_t*)&u16temp, ((uint8_t*)&u16temp)+1, USE_CRC, 1);
				if(!Ret){
					BMSData.VGPIO[i] = u16temp;
				}
			}
			tempf = (float)BMSData.VGPIO[1] * 0.1007f / 1000;
			fill_window_data(&queue_pcs_vol, tempf);
		}
		
	}
	I2C_Read16B(REG_CC_CONFIG, (uint8_t *)&BMSData.unCC_cfg, ((uint8_t *)&BMSData.unCC_cfg + 1), USE_CRC, 1);
	I2C_Read16B(REG_RD_INT0, (uint8_t *)&BMSData.RD_INT0, ((uint8_t *)&BMSData.RD_INT0 + 1), USE_CRC, 1);
//	if(BMSData.unCC_cfg.bits.cc_done)
	if((BMSData.RD_INT0 & 0x1000) == 0x1000 && (BMSData.ADC_STS&0x300) != 0x300 && (BMSData.unCC_cfg.CC_CFG&0x20) != 0x20)
	{
		//清除库伦采集中断
		I2C_Write16B(REG_INT0_CLR, 0x00, 0x10, USE_CRC, 1);
		BMSData.CCTime = SysParaVoltiage.heartBeat;
		I2C_Read16B(REG_RD_CCIRQL, (uint8_t *)&BMSData.CC_Bits, ((uint8_t *)&BMSData.CC_Bits)+1, USE_CRC, 1);
		I2C_Read16B(REG_RD_CCIRQH, ((uint8_t *)&BMSData.CC_Bits)+2, ((uint8_t *)&BMSData.CC_Bits)+3, USE_CRC, 1);				
		if(BMSData.CC_Bits&0x2000000) BMSData.CC_Temp= -(double)(0x4000000-BMSData.CC_Bits); 
		else BMSData.CC_Temp = BMSData.CC_Bits;
		BMSData.CC_Temp /= 32768;
		BMSData.CC_Temp /= 5;
		BMSData.CC_Temp /= 0.064;
		BMSData.CC_Temp /= (float)SysParaKeep.FLQ_Vol/(float)SysParaKeep.FLQ_Cur;
		BMSData.CC_Temp = - BMSData.CC_Temp;
		fill_window_data(&queue_shunt_cur, BMSData.CC_Temp);
//		I2C_Write16B(REG_CC_CONFIG, 0x02, 0x00, USE_CRC, 1); 
//		vTaskDelayUntil(&xLastWakeTime[TASK_AFE], pdMS_TO_TICKS(1));
//		I2C_Write16B(REG_CC_CONFIG, 0x03, 0x00, USE_CRC, 1); 
	}
//	else if((BMSData.unCC_cfg.bits.cc_error_sts) || ((SysParaVoltiage.heartBeat-BMSData.CCTime) > 2))
//	{
//		BMSData.CCTime = SysParaVoltiage.heartBeat;
////			I2C_Write16B(REG_CC_CONFIG, 0x03, 0x4E, USE_CRC, 1);
//		I2C_Write16B(REG_CC_CONFIG, 0x02, 0x00, USE_CRC, 1); 
//		vTaskDelayUntil(&xLastWakeTime[TASK_AFE], pdMS_TO_TICKS(1));
//		I2C_Write16B(REG_CC_CONFIG, 0x03, 0x00, USE_CRC, 1); 
//	}
	
	if(SysParaVoltiage.heartBeat - BMSData.VScanTime > 3)
	{
		delete_window_data(&queue_pcs_vol);
		SysParaVoltiage.initial_pcs_vol = 0;
		SysParaVoltiage.pcs_vol = 0;
		MP2797_Init_Normal();
	}
	else
	{
		if(Lib_Filter_Ok == calculate_mean_after_removing_extremes(&queue_pcs_vol, 10, &tempf))
		{
			SysParaVoltiage.initial_pcs_vol = tempf * 1000;
			tempf =  tempf * 501;//电压（V） = 采集原始值 / 20 * 10020
			tempf += ((float)SysParaKeep.pcs_vol_b-30000)/1000;
			tempf *= (float)SysParaKeep.pcs_vol_k /1000;
			SysParaVoltiage.pcs_vol = tempf < 3 ? 0 : tempf*10;
		}		
	}
		
	//SysParaVoltiage.Vol = (uint16_t)((BMSData.VBat)*FACTOR_K(SysParaKeep.Vol_K)*10);
	if(SysParaVoltiage.heartBeat - BMSData.CCTime > 3)
	{
		SysParaVoltiage.alarmFlag.bits.shunt_fault = 1;
		delete_window_data(&queue_shunt_cur);
		SysParaVoltiage.flq_cur = 30000;
	}
	else
	{
		if(Lib_Filter_Ok == calculate_mean_after_removing_extremes(&queue_shunt_cur, 10, &tempf))
		{
			tempf = (BMSData.CC_Temp>0?(BMSData.CC_Temp*FACTOR_K(SysParaKeep.ChaCur_K)):(BMSData.CC_Temp*FACTOR_K(SysParaKeep.DisCur_K)));
			if(GET_Bit(SysParaKeep.Hall_config, Flq_Cur1_Offset) == Bfu)
			{
				SysParaVoltiage.flq_cur = tempf*10+30000;
			}
			else
			{
				SysParaVoltiage.flq_cur = -tempf*10+30000;
			}
			SysParaVoltiage.alarmFlag.bits.shunt_fault = 0;
		}
	}
	
	vTaskDelay(pdMS_TO_TICKS(5));
}


uint32_t Res_AvrFilter(uint32_t* res_buff,uint8_t len)
{
	uint8_t i = 0;
	uint32_t res_sum = 0;
	for(i = 0;i < len;i++)
	{
		res_sum += res_buff[i];
	}
	return res_sum / len;
}








