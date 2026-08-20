#ifndef __PARAMETER_H__
#define __PARAMETER_H__

#include <stdint.h>

#define MAX_BCMU_ADDR    (32u)   /* Extended address range (1..32) */

/* Calendar time structure used by the RTC driver */
typedef struct
{
    uint16_t TimeYear;
    uint8_t  TimeMonth;
    uint8_t  TimeData;
    uint8_t  TimeHour;
    uint8_t  TimeMinute;
    uint8_t  TimeSecond;
} RTC;

/* System dynamic parameters (trimmed: Ethernet link only) */
typedef struct
{
    uint16_t ms;                    /* Millisecond counter, accumulated in SysTick */
    uint32_t ms_cnt;                /* Millisecond total counter */
    uint32_t heartBeat;             /* Heartbeat counter, +1 per second */
} SysParaVoltiage_st;

extern volatile SysParaVoltiage_st SysParaVoltiage;

/* Network configuration parameters */
typedef struct
{
    uint8_t  ipaddr[4];             /* IP address */
    uint8_t  netmask[4];            /* Network mask */
    uint8_t  gw[4];                 /* Gateway address */
    uint8_t  ExtAddr;               /* Extended address (offset of last IP octet, 1..32) */
    uint8_t  TestModeEnable;        /* Test mode flag (1 = fixed IP 192.168.1.21) */
} SysParaKeep_st;

extern SysParaKeep_st SysParaKeep;

void initValue(void);
void CheckPara(void);

#endif /* __PARAMETER_H__ */
