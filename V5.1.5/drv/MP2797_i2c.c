#include "main.h"

const uint8_t CRC8Table[256] = {
0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

uint8_t CRC8(uint8_t* data, uint16_t length)
{
	uint16_t crc= 0x00;
	uint16_t i,j;
	for (i=0; i<length; i++)
	{
		j = (crc ^ *data++) & 0xFF;
		crc = (CRC8Table[j] ^ (crc << 8)) & 0xFF;
	}
	return (uint8_t) (crc & 0xFF);
}

#define SCL_PORT             (GPIO_PORT_D)        
#define SCL_PIN              (GPIO_PIN_04)        
#define SDA_PORT             (GPIO_PORT_D)       
#define SDA_PIN              (GPIO_PIN_05)   

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

#define SCL_dir(x)	Port_dir(SCL_PORT, SCL_PIN, x);
#define SDA_dir(x)	Port_dir(SDA_PORT, SDA_PIN, x);
#define SCL(x)		x?GPIO_SetPins(SCL_PORT, SCL_PIN):GPIO_ResetPins(SCL_PORT, SCL_PIN);
#define SDA(x)		x?GPIO_SetPins(SDA_PORT, SDA_PIN):GPIO_ResetPins(SDA_PORT, SDA_PIN);
#define SDA_READ	GPIO_ReadInputPins(SDA_PORT, SDA_PIN)

void I2C_Initialize(void)
{
	SCL_dir(1);
	SDA_dir(1);
	SCL(1);
	SDA(1);
}

static void I2Cdelay(void)
{
	unsigned int i;
	for(i=0; i<90; i++)
	{
		;
	}
}

static void I2Cstart(void)
{
	SDA_dir(1);
	SDA(1);
	SCL(1);
	I2Cdelay();
	SDA(0);
	I2Cdelay();
	SCL(0);
	I2Cdelay();
}

static uint8_t I2Creadack(void)
{
	uint8_t ret = 0;
	uint8_t cnt = 20;
	
	SCL(0);
	I2Cdelay();
	SCL(1);
	I2Cdelay();
	SDA_dir(0);
	
	while(cnt--&&ret==0)
	{
		if(SDA_READ == 1)
		{
			ret = 1;
		}
		else
		{
			ret = 0;
		}
	}
	SCL(0);
	SDA_dir(1);
	SDA(1);
	I2Cdelay();
	
	return ret;
}

static void I2Csend(uint8_t data)
{
	uint8_t i;
	SDA_dir(1);
	for(i=0; i<8; i++)
	{
		SCL(0);
		if((data&0x80)>>7)
		{
			SDA(1);
		}
		else
		{
			SDA(0);
		}
		I2Cdelay();
		SCL(1);
		I2Cdelay();
		data<<=1;
	}
}

static void I2Csendack(void)
{
	SCL(0);
	I2Cdelay();
	SDA_dir(1);
	SDA(0);
	I2Cdelay();
	SCL(1);
	I2Cdelay();
	SCL(0);
	I2Cdelay();	
}

static void I2Csendnack(void)
{
	SCL(0);
	I2Cdelay();
	SDA_dir(1);
	SDA(1);
	I2Cdelay();
	SCL(1);
	I2Cdelay();
	SCL(0);
	I2Cdelay();	
}

static uint8_t I2Creceive(void)
{
	uint8_t i,data = 0;

	SDA_dir(0);
	for(i=0; i<8; i++)
	{
		I2Cdelay();
		SCL(0);
		I2Cdelay();
		SCL(1);
		I2Cdelay();
		data<<=1;
		if(SDA_READ == 1)
		{
			data|=1;
		}
	}
	return data;
}

static void I2Cstop(void)
{
	SDA_dir(1);
	SCL(0);
	SDA(0);
	I2Cdelay();
	SCL(1);
	I2Cdelay();
	SDA(1);
}

void I2C_Write16B(uint8_t Register, uint8_t DataL, uint8_t DataH, uint8_t usecrc, uint8_t add_delay)
{
	uint8_t CRCInput[4], CRCResult;
	
	if (add_delay) DelayNus(10);
	if (usecrc)
	{
		CRCInput[0]=(0x01<<1);							 
		CRCInput[1]=Register;						 
		CRCInput[2]=DataL;							 
		CRCInput[3]=DataH;								 
		CRCResult=CRC8(CRCInput,4);
	}
	I2Cstart();		
	I2Csend((unsigned char)(0x01<<1u)|0x00);
	(void)I2Creadack();
	I2Csend(Register);
	(void)I2Creadack();	
	I2Csend(DataL);		
	(void)I2Creadack();	
	I2Csend(DataH);		
	(void)I2Creadack();	
	if(usecrc)
	{
		I2Csend(CRCResult);								 
		(void)I2Creadack();								 
	}	
	I2Cstop();		
}

uint8_t I2C_Read16B(uint8_t Register, uint8_t* DataL, uint8_t* DataH, uint8_t usecrc, uint8_t add_delay)
{
	uint8_t CRCInput[6],CRCResult,BuffH,BuffL,CRCReceived,u8Ret = 1;
	
	if (add_delay) DelayNus(10);
	if (usecrc)
	{
		CRCInput[0]=(0x01<<1);						
		CRCInput[1]=Register;								
		CRCInput[2]=(0x01<<1)|0x01;					
		CRCInput[3]=Register;								
	}
	I2Cstart();	
	I2Csend((unsigned char)(0x01<<1u)|0x00);
	(void)I2Creadack();			
	I2Csend(Register);	
	(void)I2Creadack();	
	I2Cdelay();	
	I2Cstart(); 
	I2Csend((unsigned char)(0x01<<1u)|0x01);
	(void)I2Creadack();		
	if(usecrc)
	{
		BuffL = I2Creceive();							
		I2Csendack();									
		BuffH = I2Creceive();								
		I2Csendack();										
		CRCReceived = I2Creceive();							
		I2Csendnack();										
		I2Cstop();											
		CRCInput[4]=BuffL;
		CRCInput[5]=BuffH;
		CRCResult=CRC8(CRCInput,6);
		if (CRCResult==CRCReceived)
		{
			*DataL = BuffL;
			*DataH = BuffH;
			u8Ret = 0;
		}
	}
	else
	{
		BuffL = I2Creceive();							
		I2Csendack();										
		BuffH = I2Creceive();						
		I2Csendnack();										
		I2Cstop();										
		*DataL = BuffL;
		*DataH = BuffH;
		u8Ret = 0;
	} 
	return u8Ret;	
}
















