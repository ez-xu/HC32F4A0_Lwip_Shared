#ifndef DRV_ADC_H__
#define DRV_ADC_H__

#include "stdint.h"

/* ADC reference voltage. The voltage of pin VREFH. */
#define ADC_VREF                        (3.3F)

/* ADC accuracy(according to the resolution of ADC). */
#define ADC_ACCURACY                    (1UL << 12U)

/* Calculate the voltage(mV). */
#define ADC_CAL_VOL(adcVal)             (uint16_t)((((float32_t)(adcVal) * ADC_VREF) / ((float32_t)ADC_ACCURACY)) * 1000.F)


#define ADC_CLK_SYS_CLK                 (1U)
#define ADC_CLK_PLLH                    (2U)
#define ADC_CLK_PLLA                    (3U)

/*
 * Selects a clock source according to the application requirements.
 * PCLK4 is the clock for digital interface.
 * PCLK2 is the clock for analog circuit.
 * PCLK4 and PCLK2 are synchronous when the clock source is PLL.
 * PCLK4 : PCLK2 = 1:1, 2:1, 4:1, 8:1, 1:2, 1:4.
 * PCLK2 is in range [1MHz, 60MHz].
 * If the system clock is selected as the ADC clock, macro 'ADC_ADC_CLK' can only be defined as 'CLK_PERIPHCLK_PCLK'.
 * If PLLH is selected as the ADC clock, macro 'ADC_ADC_CLK' can be defined as 'CLK_PERIPHCLK_PLLx'(x=Q, R).
 * If PLLA is selected as the ADC clock, macro 'ADC_ADC_CLK' can be defined as 'CLK_PERIPHCLK_PLLXx'(x=P, Q, R).
 */

#define ADC_CLK_SEL                     (ADC_CLK_SYS_CLK)

#if (ADC_CLK_SEL == ADC_CLK_SYS_CLK)
#define ADC_CLK                         (CLK_PERIPHCLK_PCLK)

#elif (ADC_CLK_SEL == ADC_CLK_PLLH)
#define ADC_CLK                         (CLK_PERIPHCLK_PLLQ)

#elif (ADC_CLK_SEL == ADC_CLK_PLLA)
#define ADC_CLK                         (CLK_PERIPHCLK_PLLXP)

#else
#error "The clock source your selected does not exist!!!"
#endif

/* ADC unit instance for this example. */
#define ADC_UNIT                        (CM_ADC1)
#define ADC_PERIPH_CLK                  (FCG3_PERIPH_ADC1)

/* Selects ADC channels that needed. */
#define ADC_CH10_PORT                   (GPIO_PORT_C)
#define ADC_CH10_PIN                    (GPIO_PIN_00)

#define ADC_CH5_PORT                    (GPIO_PORT_A)
#define ADC_CH5_PIN                     (GPIO_PIN_05)

#define ADC_CH6_PORT                    (GPIO_PORT_A)
#define ADC_CH6_PIN                     (GPIO_PIN_06)

/* ADC sequence to be used. */
#define ADC_SEQ                         (ADC_SEQ_A)
/* Flag of conversion end. */
#define ADC_EOC_FLAG                    (ADC_FLAG_EOCA)

/* Timeout value. */
#define ADC_TIMEOUT_VAL                 (1000U)

uint16_t AdcPolling(uint8_t CHx);
void AdcConfig(void);

#endif
