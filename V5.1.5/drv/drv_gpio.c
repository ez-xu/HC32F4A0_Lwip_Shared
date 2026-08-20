#include "drv_gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "parameter.h"


SemaphoreHandle_t semCounting_watchdog; 

void drv_gpio_init(void)
{
	stc_gpio_init_t stcGpioInit;

	GPIO_SetDebugPort(GPIO_PIN_TDI,DISABLE); 
	GPIO_SetDebugPort(GPIO_PIN_TRST,DISABLE);  	
	GPIO_SetDebugPort(GPIO_PIN_TDO,DISABLE);  	

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState = PIN_STAT_SET;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;	
	
	(void)GPIO_Init(LEDRUN_PORT, LEDRUN_PIN, &stcGpioInit);
	(void)GPIO_Init(LEDERR_PORT, LEDERR_PIN, &stcGpioInit);	
	(void)GPIO_Init(WTD_PORT, WTD_PIN, &stcGpioInit);	
	(void)GPIO_Init(I2C_WAKEUP_PORT, I2C_WAKEUP_PIN, &stcGpioInit);
	(void)GPIO_Init(EVOUT_PORT, EVOUT_PIN, &stcGpioInit);
	(void)GPIO_Init(DO1_PORT, DO1_PIN, &stcGpioInit);
	(void)GPIO_Init(DO2_PORT, DO2_PIN, &stcGpioInit);
	(void)GPIO_Init(DO3_PORT, DO3_PIN, &stcGpioInit);
	(void)GPIO_Init(DO4_PORT, DO4_PIN, &stcGpioInit);
	(void)GPIO_Init(DO5_PORT, DO5_PIN, &stcGpioInit);
	(void)GPIO_Init(DO6_PORT, DO6_PIN, &stcGpioInit);
	(void)GPIO_Init(DO7_PORT, DO7_PIN, &stcGpioInit);
	(void)GPIO_Init(S_A_PORT, S_A_PIN, &stcGpioInit);
	(void)GPIO_Init(S_B_PORT, S_B_PIN, &stcGpioInit);
	(void)GPIO_Init(S_C_PORT, S_C_PIN, &stcGpioInit);	
	
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;	
	(void)GPIO_Init(JY_K1_PORT, JY_K1_PIN, &stcGpioInit);
	(void)GPIO_Init(JY_K2_PORT, JY_K2_PIN, &stcGpioInit);
	(void)GPIO_Init(JY_K3_PORT, JY_K3_PIN, &stcGpioInit);
	JY_K1_SET();
	JY_K2_SET();
	JY_K3_RST();
	(void)GPIO_Init(ADDR_PORT, ADDR_PIN, &stcGpioInit);	
	
    (void)GPIO_StructInit(&stcGpioInit);    
    stcGpioInit.u16PinDir = PIN_DIR_IN;             
    stcGpioInit.u16PullUp = PIN_PU_ON;    

	(void)GPIO_Init(DI1_PORT, DI1_PIN, &stcGpioInit); 	
	(void)GPIO_Init(DI2_PORT, DI2_PIN, &stcGpioInit); 	
	(void)GPIO_Init(DI3_PORT, DI3_PIN, &stcGpioInit); 	
	(void)GPIO_Init(DI4_PORT, DI4_PIN, &stcGpioInit); 	
	(void)GPIO_Init(DI5_PORT, DI5_PIN, &stcGpioInit); 	
	(void)GPIO_Init(DI6_PORT, DI6_PIN, &stcGpioInit); 	
	(void)GPIO_Init(DI7_PORT, DI7_PIN, &stcGpioInit); 	
	(void)GPIO_Init(DI8_PORT, DI8_PIN, &stcGpioInit); 	
	
	semCounting_watchdog = xSemaphoreCreateCounting(1,1);   //最大1   当前1
}

void WTD_Reset(void)
{
	WTD_RST();
    __asm("nop");__asm("nop");__asm("nop");__asm("nop");
    __asm("nop");__asm("nop");__asm("nop");__asm("nop");
	WTD_SET();
}

void reset_external_dog(void)
{
//	if (xSemaphoreTake(semCounting_watchdog,(TickType_t)0) == pdTRUE) 
//	{//0-无等待 轮询
		WTD_Reset();
//		xSemaphoreGive(semCounting_watchdog);
//	}
}

