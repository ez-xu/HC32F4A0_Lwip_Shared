#include "drv_ads1113.h"
#include "string.h"
#include "hc32_ll.h"
#include "parameter.h"

#define IIC_SDA_PORT		(GPIO_PORT_D)        
#define IIC_SDA_PIN			(GPIO_PIN_07) 
#define IIC_SCL_PORT		(GPIO_PORT_D)        
#define IIC_SCL_PIN			(GPIO_PIN_06) 

#define SDA1_DDR_OUT  			Port_dir(IIC_SDA_PORT, IIC_SDA_PIN, 1);
#define SDA1_DDR_IN				Port_dir(IIC_SDA_PORT, IIC_SDA_PIN, 0); 
#define SDA1				 	GPIO_ReadInputPins(IIC_SDA_PORT, IIC_SDA_PIN)
#define SDA1_1  			 	GPIO_SetPins(IIC_SDA_PORT,IIC_SDA_PIN)
#define SDA1_0  			 	GPIO_ResetPins(IIC_SDA_PORT,IIC_SDA_PIN)
#define SCL1_DDR_OUT			Port_dir(IIC_SCL_PORT, IIC_SCL_PIN, 1);
#define SCL1_1				 	GPIO_SetPins(IIC_SCL_PORT, IIC_SCL_PIN)	
#define SCL1_0     		 		GPIO_ResetPins(IIC_SCL_PORT, IIC_SCL_PIN)

#define ADS1113_ADDR 			0x90
#define ADS1113_READ			0x01
#define ADS1113_WRITE			0x00
#define ADS1113_CONVERSIONREG	0x00
#define ADS1113_CONFIGREG		0x01
#define ADS1113_ACK				0
#define ADS1113_NACK			1







static void Port_dir(uint8_t u8Port, uint16_t u16Pin, uint8_t mode)
{
	stc_gpio_init_t stcGpioInit;	

	(void)GPIO_StructInit(&stcGpioInit);    
	if(mode == 1)
	{
		stcGpioInit.u16PinDir = PIN_DIR_OUT;	
	}
	else
	{
		stcGpioInit.u16PinDir = PIN_DIR_IN;               
	}
	(void)GPIO_Init(u8Port, u16Pin, &stcGpioInit);
}

void AD1_Delay(uint16_t cnt)
{
	uint8_t i;

	while(cnt--)
	{
		for(i=0; i<29; i++);
	}
}

void AD1_Start(void)
{
	SCL1_DDR_OUT;
	SDA1_DDR_OUT;
	AD1_Delay(5);
	SCL1_1;
	SDA1_1;
	AD1_Delay(10);
	SDA1_0;
	AD1_Delay(5);
	SCL1_0;
	AD1_Delay(5);
}

void AD1_Stop(void)
{
	SCL1_DDR_OUT;
	SDA1_DDR_OUT;
	AD1_Delay(5);
	SCL1_0;
	SDA1_0;
	AD1_Delay(10);
	SCL1_1;
	AD1_Delay(5);
	SDA1_1;
	AD1_Delay(5);
}

uint8_t AD1_ReadOneBit(void)
{
	uint8_t Ack;

	SDA1_DDR_IN;
	AD1_Delay(5);
	SCL1_1;
	if(SDA1) Ack = 1;
	else Ack = 0;
	AD1_Delay(5);
	SCL1_0;
	AD1_Delay(5);

	return Ack;
}

void AD1_SendOneBit(uint8_t ack)
{
	SDA1_DDR_OUT;
	AD1_Delay(5);
	if(!ack) SDA1_0;
	else SDA1_1;
	SCL1_1;
	AD1_Delay(5);
	SCL1_0;
	AD1_Delay(5);
}

uint8_t AD1_ReadOneByte(uint8_t ack)
{
	uint8_t i,dat = 0;

	SDA1_DDR_IN;
	AD1_Delay(5);
	for(i=0; i<8; i++)
	{
		dat <<= 1;
		SCL1_1;
		if(SDA1)
		{
			dat |= 1;
		}
		AD1_Delay(5);
		SCL1_0;
		AD1_Delay(5);
	}
	AD1_SendOneBit(ack);

	return dat;
}

uint8_t AD1_SendOneByte(uint8_t dat)
{
	uint8_t i,ack;

	SDA1_DDR_OUT;
	AD1_Delay(5);
	for(i=0; i<8; i++)
	{
		if(dat & 0x80) SDA1_1;
		else SDA1_0;
		AD1_Delay(10);
		SCL1_1;
		AD1_Delay(5);
		SCL1_0;
		AD1_Delay(5);
		dat <<= 1;
	}
	ack = AD1_ReadOneBit();

	return ack;
}

uint8_t AD1_ConfigRegInit(void)
{
	uint8_t i,err=0,RegBuff[4];

	RegBuff[0] = ADS1113_ADDR | ADS1113_WRITE;    
	RegBuff[1] = ADS1113_CONFIGREG;			      
	RegBuff[2] = 0x84;							 
	RegBuff[3] = 0xE0;							 
	AD1_Start();
	for(i=0; i<4; i++)
	{
		err = AD1_SendOneByte(RegBuff[i]);
		if(err) 
		{	
			return err;
		}
	}
	AD1_Stop();
	return err;
}

uint8_t AD1_ConversionRegPointerInit(void)
{
	uint8_t RegBuff[2],i,err=0;

	RegBuff[0] = ADS1113_ADDR | ADS1113_WRITE;
	RegBuff[1] = ADS1113_CONVERSIONREG;
	AD1_Start();
	for(i=0; i<2; i++)
	{
		err = AD1_SendOneByte(RegBuff[i]);
		if(err) 
		{	
			return err;
		}
	}
	AD1_Stop();
	return err;
}

uint8_t AD1_ReadConversionValue(uint16_t *val)
{
	
	uint8_t buff[2],err=0;

	AD1_Start();
	err = AD1_SendOneByte(ADS1113_ADDR | ADS1113_READ);
	if(err) 
	{	
		return err;
	}
	buff[0] = AD1_ReadOneByte(ADS1113_ACK);
	buff[1] = AD1_ReadOneByte(ADS1113_ACK);
	AD1_Stop();

	*val = buff[0]*256 + buff[1];

	return err;
}

uint8_t AD1_GetValue(uint16_t *val)
{
	uint8_t err = 0;
	err = AD1_ConfigRegInit();
	if(err)
	{
		return err;
	}
	err = AD1_ConversionRegPointerInit();
	if(err)
	{
		return err;
	}
	err = AD1_ReadConversionValue(val);
	if(err)
	{
		return err;
	}
	return err;
}

uint8_t ADS110A0_GetValue(uint16_t *val)
{
	uint8_t RegBuff[2],i,err=0;

	RegBuff[0] = ADS1113_ADDR | ADS1113_WRITE;
	RegBuff[1] = 0x0C;
	AD1_Start();
	for(i=0; i<2; i++)
	{
		err = AD1_SendOneByte(RegBuff[i]);
		if(err) 
		{
			return err;
		}
	}
	AD1_Stop();
	err = AD1_ReadConversionValue(val);
	if(err)
	{
		return err;
	}
	return err;
}

uint8_t ADS1113_V_Read(float *u)
{
	uint16_t val_u, val;

	if(SysParaVoltiage.HardwareVer == 0)
	{
		if(ADS110A0_GetValue(&val_u) == 1)
		{
			return 1;
		}
	}
	else
	{
		if(AD1_GetValue(&val_u) == 1)
		{
			return 1;
		}
	}
	
	if(val_u & 0x8000)
	{
		val = 0x8000 - (val_u & 0x7FFF);
		*u = -(2.048 * val / 0x7FFF );
	}
	else
	{
		val = (val_u & 0x7FFF);
		*u = (float)2.048 * val/ 0x7FFF;
	}
	return 0;
}
