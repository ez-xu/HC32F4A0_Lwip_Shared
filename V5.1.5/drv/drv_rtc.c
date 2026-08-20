#include "drv_rtc.h"
#include "hc32_ll.h"

uint8_t u8SecIntFlag = 0U;

uint8_t RTC_CalendarConfig(RTC time)
{
    stc_rtc_date_t stcRtcDate;
    stc_rtc_time_t stcRtcTime;

    /* Date configuration */
    stcRtcDate.u8Year    = time.TimeYear - 2000;
    stcRtcDate.u8Month   = time.TimeMonth;
    stcRtcDate.u8Day     = time.TimeData;
    stcRtcDate.u8Weekday = 0;

    /* Time configuration */
    stcRtcTime.u8Hour   = time.TimeHour;
    stcRtcTime.u8Minute = time.TimeMinute;
    stcRtcTime.u8Second = time.TimeSecond;
	stcRtcTime.u8AmPm   = RTC_HOUR_12H_PM;

    if (LL_OK != RTC_SetDate(RTC_DATA_FMT_DEC, &stcRtcDate)) {
        DDL_Printf(PRINT_LEVEL_ERROR, "Set Date failed!\r\n");
			  return 1;
    }

    if (LL_OK != RTC_SetTime(RTC_DATA_FMT_DEC, &stcRtcTime)) {
        DDL_Printf(PRINT_LEVEL_ERROR, "Set Time failed!\r\n");
				return 2;
    }
	return 0;
}

static void RTC_Period_IrqCallback(void)
{
    u8SecIntFlag = 1;
    RTC_ClearStatus(RTC_FLAG_PERIOD);
	//获取时间
	if(u8SecIntFlag == 1)
	{
		RTC rtc_time;
	GetRtcTime(&rtc_time);
	}
}

void RTC_Config(void)
{
    int32_t i32Ret;
    stc_rtc_init_t stcRtcInit;
    stc_irq_signin_config_t stcIrqSignConfig;

    /* RTC period interrupt configure */
    stcIrqSignConfig.enIntSrc    = INT_SRC_RTC_PRD;
    stcIrqSignConfig.enIRQn      = INT052_IRQn;
    stcIrqSignConfig.pfnCallback = &RTC_Period_IrqCallback;
    (void)INTC_IrqSignOut(stcIrqSignConfig.enIRQn);
    i32Ret = INTC_IrqSignIn(&stcIrqSignConfig);
    if (LL_OK != i32Ret) {
        /* check parameter */
        for (;;) {
        }
    }

    /* Clear pending */
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    /* Set priority */
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    /* Enable NVIC */
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);

    /* RTC stopped */
    if (DISABLE == RTC_GetCounterState()) {
        /* Reset the VBAT area */
        PWC_VBAT_Reset();
        /* Reset RTC counter */
        if (LL_ERR_TIMEOUT == RTC_DeInit()) {
            DDL_Printf(PRINT_LEVEL_ERROR, "Reset RTC failed!\r\n");
        } else {
            /* Configure structure initialization */
            (void)RTC_StructInit(&stcRtcInit);

            /* Configuration RTC structure */
            stcRtcInit.u8ClockSrc   = RTC_CLK_SRC_LRC;
            stcRtcInit.u8HourFormat = RTC_HOUR_FMT_24H;
            stcRtcInit.u8IntPeriod  = RTC_INT_PERIOD_PER_SEC;
            (void)RTC_Init(&stcRtcInit);

            /* Update date and time */
//            RTC_CalendarConfig();
            /* Enable period interrupt */
            RTC_IntCmd(RTC_INT_PERIOD, ENABLE);
            /* Startup RTC count */
            RTC_Cmd(ENABLE);
        }
    }
	u8SecIntFlag = 1;
	RTC rtc_time;
	GetRtcTime(&rtc_time);
}



void GetRtcTime(RTC *time)
{
	stc_rtc_date_t stcCurrentDate = {0};
	stc_rtc_time_t stcCurrentTime = {0};
	if (1U == u8SecIntFlag) 
	{
		u8SecIntFlag = 0U;
		/* Get current date */
		if (LL_OK == RTC_GetDate(RTC_DATA_FMT_DEC, &stcCurrentDate)) 
		{
			/* Get current time */
			if (LL_OK == RTC_GetTime(RTC_DATA_FMT_DEC, &stcCurrentTime)) {
					time->TimeYear = stcCurrentDate.u8Year+2000;
					time->TimeMonth = stcCurrentDate.u8Month;
					time->TimeData = stcCurrentDate.u8Day;
					time->TimeHour = stcCurrentTime.u8Hour;
					time->TimeMinute = stcCurrentTime.u8Minute;
					time->TimeSecond = stcCurrentTime.u8Second;
					/* Print current date and time */
					DDL_Printf(PRINT_LEVEL_ERROR, "20%02d/%02d/%02d %02d:%02d:%02d ", stcCurrentDate.u8Year, stcCurrentDate.u8Month,
										 stcCurrentDate.u8Day, stcCurrentTime.u8Hour,
										 stcCurrentTime.u8Minute, stcCurrentTime.u8Second);
			} else {
					DDL_Printf(PRINT_LEVEL_ERROR, "Get time failed!\r\n");
			}
		}
		else
		{
			DDL_Printf(PRINT_LEVEL_ERROR, "Get date failed!\r\n");
		}
	}

}
