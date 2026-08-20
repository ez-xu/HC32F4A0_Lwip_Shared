#include "main.h"
#include "drv/drv_gpio.h"

Record_st Record;
uint8_t Record_Flag = 0;
uint32_t Record_Time = 0;

uint8_t Record_Test = 0; //调试用

void MP2797_Init_Fast(void)
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
	}
	while(I2C_Read16B(REG_COMM_CONFIG, &DataRx[0], &DataRx[1], USE_CRC, 1));
	BMSData.ChipSystemError = 0;
	
	I2C_Write16B(0x99, 0x00, 0x00, USE_CRC, 1); 	
	I2C_Write16B(0xB3, 0x00, 0x00, USE_CRC, 1); 
	I2C_Write16B(0xEF, 0x00, 0x00, USE_CRC, 1); 
	
	I2C_Write16B(0x0C, 0x03, 0x00, USE_CRC, 1); //gpio1 in

	I2C_Write16B(0x05, 0x00, 0x00, USE_CRC, 1); 	
	I2C_Write16B(0x07, 0x00, 0x00, USE_CRC, 1); 
	I2C_Write16B(0x99, 0x00, 0x00, USE_CRC, 1); 
	I2C_Write16B(0x9E, 0x01, 0x00, USE_CRC, 1); //gpio1_read_en
	I2C_Write16B(0x9D, 0x01, 0x00, USE_CRC, 1); //cell_1_vi_read_en  
	I2C_Write16B(0x9C, 0x0B, 0x01, USE_CRC, 1); //scan_vcells_en & scan_gpio1_en  
	I2C_Write16B(0x47, 0x00, 0x00, USE_CRC, 1); 
	
	I2C_Write16B(0xEE, 0x86, 0x86, USE_CRC, 1); 
	I2C_Write16B(0xEF, 0x01, 0x00, USE_CRC, 1); 
	I2C_Write16B(0x99, 0x01, 0x00, USE_CRC, 1); //adc_scan_go
	vTaskDelay(pdMS_TO_TICKS(8));
	I2C_Write16B(0xB3, 0x04, 0x00, USE_CRC, 1); 
	vTaskDelay(pdMS_TO_TICKS(12));
}

void MP2797_Comm_Fast(void)
{
	uint8_t Ret;
	uint16_t u16temp;
	//static uint16_t j = 0;
	int16_t s16temp;
	float tempf;
	
//	JY_K1_SET();
//	JY_K2_RST();	
//	SysParaVoltiage.alarmFlag.bits.ResComplete = 1;
	
	Ret = I2C_Read16B(REG_RD_VCELL1, (uint8_t*)&u16temp, ((uint8_t*)&u16temp)+1, USE_CRC, 1);
	if(!Ret){
		BMSData.VCELL[0] = u16temp;
	}
//	Ret = I2C_Read16B(REG_RD_VGPIO1, (uint8_t*)&u16temp, ((uint8_t*)&u16temp)+1, USE_CRC, 1);
//	if(!Ret){
//		BMSData.VGPIO[0] = u16temp;
//	}
	Ret = I2C_Read16B(REG_RD_ICELL1, (uint8_t*)&s16temp, ((uint8_t*)&s16temp)+1, USE_CRC, 1);
	if(!Ret){
		BMSData.ICELL1 = s16temp;
	}	
	vTaskDelay(pdMS_TO_TICKS(1));
	
	//BMSData.VBat = VOL_CALCULATE(BMSData.VGPIO[0]);
	//SysParaVoltiage.Vol = (uint16_t)((BMSData.VBat)*FACTOR_K(SysParaKeep.Vol_K)*10);		
	tempf = (double)BMSData.ICELL1*0.00305176;
	tempf = tempf/((float)SysParaKeep.FLQ_Vol/(float)SysParaKeep.FLQ_Cur);
	tempf = (tempf<0?(tempf*FACTOR_K(SysParaKeep.ChaCur_K)):(tempf*FACTOR_K(SysParaKeep.DisCur_K)));
	SysParaVoltiage.flq_cur = -tempf*10+30000;

	//if(SysParaVoltiage.heartBeat > 60) SysParaVoltiage.alarmFlag.bits.ResComplete = 1;	

//	if(Record_Flag == 0)
//	{
//		if((((SysParaVoltiage.Cur > 31000) || (SysParaVoltiage.Cur < 29000))
//		&& ((SysParaVoltiage.heartBeat - Record_Time) > 30))
//		|| (Record_Test == 1))
//		{
//			Record_Test = 0;
//			Record_Flag = 1;
//			j = RECORD_LEN / 2;
//		}
//		else 
//		{
//			j = 1;
//		}
//	}
//	else if(Record_Flag == 1)
//	{
//		if(j == 0)
//		{
//			//保存数据
//			Store_Record();
//			LogParaKeep.LogCnt++;
//			Store_LogParaKeep();
//			Record_Flag = 0;
//			Record_Time = SysParaVoltiage.heartBeat;
//		}
//	}
//	
//	if(j > 0)
//	{
//		j--;
//		for(i=0; i<(RECORD_LEN-1); i++)
//		{
//			Record.vol[i] = Record.vol[i+1];
//			Record.cur[i] = Record.cur[i+1];
//		}
//		Record.vol[i] = SysParaVoltiage.Vol;
//		Record.cur[i] = SysParaVoltiage.Cur;
//	}
}

//void Store_Record(void)
//{
//	uint8_t ret=0;
//	uint16_t i,j,Timeout;
//	uint32_t data_write;
//	en_int_status_t flag1,flag2;
//	uint8_t erase = 0;
//	
//	taskENTER_CRITICAL();
//	for(i=0; i<sizeof(Record_st); i+=8)
//	{
//		WTD_Reset();
//		/* Wait flash0, flash1 ready. */
//		Timeout = 0;
//		do {
//			flag1 = EFM_GetStatus(EFM_FLAG_RDY);
//			flag2 = EFM_GetStatus(EFM_FLAG_RDY1);
//			Timeout++;
//			if(Timeout > 0x1000)
//			{
//				ret = 2;
//				break;
//			}
//		} while ((SET != flag1) || (SET != flag2));	
//		if(ret == 2)
//		{
//			break;
//		}
//		/* EFM_FWMC wirte enable */
//		EFM_FWMC_Cmd(ENABLE);	
//		if(erase == 0)
//		{
//			erase = 1;
//			(void)EFM_SingleSectorOperateCmd((FLASH_RECORD_START/FLASH_SECTOR_SIZE) + (LogParaKeep.LogCnt%100), ENABLE);
//			EFM_SectorErase(FLASH_RECORD_START + ((LogParaKeep.LogCnt%100)*FLASH_SECTOR_SIZE));
//		}
//		for(j=0; j<2; j++)
//		{
//			data_write  = ((uint32_t)(*(((uint8_t*)&Record)+i+j*4+3))<<24)&0xff000000;
//			data_write |= ((uint32_t)(*(((uint8_t*)&Record)+i+j*4+2))<<16)&0x00ff0000;
//			data_write |= ((uint32_t)(*(((uint8_t*)&Record)+i+j*4+1))<< 8)&0x0000ff00;
//			data_write |= ((uint32_t)(*(((uint8_t*)&Record)+i+j*4+0))<< 0)&0x000000ff;
//			(void)EFM_SingleSectorOperateCmd((FLASH_RECORD_START/FLASH_SECTOR_SIZE) + (LogParaKeep.LogCnt%100), ENABLE);
//			EFM_ProgramWord((FLASH_RECORD_START + ((LogParaKeep.LogCnt%100)*FLASH_SECTOR_SIZE))+i+j*4,data_write);
//		}
//		EFM_FWMC_Cmd(DISABLE);
//	}	
//	taskEXIT_CRITICAL();
//}




