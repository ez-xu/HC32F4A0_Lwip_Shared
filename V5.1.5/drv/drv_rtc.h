#ifndef DRV_RTC_H__
#define DRV_RTC_H__

#include "parameter.h"
#include "hc32_ll_rtc.h"

extern uint8_t u8SecIntFlag;

void RTC_Config(void);
uint8_t RTC_CalendarConfig(RTC time);
void GetRtcTime(RTC *time);

#endif
