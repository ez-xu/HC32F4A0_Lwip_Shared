#include "drv_adc.h"
#include "drv/drv_gpio.h"



void AdcSetPinAnalogMode(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(ADC_CH10_PORT, ADC_CH10_PIN, &stcGpioInit);
	(void)GPIO_Init(ADC_CH5_PORT, ADC_CH5_PIN, &stcGpioInit);
	(void)GPIO_Init(ADC_CH6_PORT, ADC_CH6_PIN, &stcGpioInit);
}

void AdcInitConfig(void)
{
    stc_adc_init_t stcAdcInit;

    /* 1. Enable ADC peripheral clock. */
    FCG_Fcg3PeriphClockCmd(ADC_PERIPH_CLK, ENABLE);

    /* 2. Modify the default value depends on the application. Not needed here. */
    (void)ADC_StructInit(&stcAdcInit);

    /* 3. Initializes ADC. */
    (void)ADC_Init(ADC_UNIT, &stcAdcInit);

    /* 4. ADC channel configuration. */
    /* 4.1 Set the ADC pin to analog input mode. */
    AdcSetPinAnalogMode();
    /* 4.2 Enable ADC channels. Call ADC_ChCmd() again to enable more channels if needed. */
    ADC_ChCmd(ADC_UNIT, ADC_SEQ, ADC_CH10, ENABLE);
	ADC_ChCmd(ADC_UNIT, ADC_SEQ, ADC_CH5, ENABLE);
	ADC_ChCmd(ADC_UNIT, ADC_SEQ, ADC_CH6, ENABLE);
	
    /* 5. Conversion data average calculation function, if needed.
          Call ADC_ConvDataAverageChCmd() again to enable more average channels if needed. */
    ADC_ConvDataAverageConfig(ADC_UNIT, ADC_AVG_CNT8);
    ADC_ConvDataAverageChCmd(ADC_UNIT, ADC_CH10, ENABLE);
	ADC_ConvDataAverageChCmd(ADC_UNIT, ADC_CH5, ENABLE);
	ADC_ConvDataAverageChCmd(ADC_UNIT, ADC_CH6, ENABLE);
}



void AdcConfig(void)
{
	//AdcClockConfig();
	AdcInitConfig();
}

uint16_t AdcPolling(uint8_t CHx)
{
    uint16_t u16AdcValue = 0,ad[30]={0},i,j;
    int32_t iRet = LL_ERR;
    __IO uint32_t u32TimeCount = 0UL;

    /* Can ONLY start sequence A conversion.
       Sequence B needs hardware trigger to start conversion. */
    for (i=0; i<30; i++)
	{
		ADC_Start(ADC_UNIT);
		do {
			if (ADC_GetStatus(ADC_UNIT, ADC_EOC_FLAG) == SET) {
				ADC_ClearStatus(ADC_UNIT, ADC_EOC_FLAG);
				iRet = LL_OK;
				break;
			}
		} while (u32TimeCount++ < ADC_TIMEOUT_VAL);

		if (iRet == LL_OK) {
			/* Get any ADC value of sequence A channel that needed. */
			u16AdcValue = ADC_GetValue(ADC_UNIT, CHx);
		} else {
			ADC_Stop(ADC_UNIT);
		}
		ad[i] = u16AdcValue;
	}
	for (j=0; j<15; j++)
	{
		for (i=j; i<30; i++)
		{
			if(ad[i] > ad[j])
			{
				ad[i] ^= ad[j];
				ad[j] ^= ad[i];
				ad[i] ^= ad[j];
			}
		}
	}
	
	return ad[14];//u16AdcValue;
}
