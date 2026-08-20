#ifndef __MAIN_H__
#define __MAIN_H__

#include "hc32_ll.h"
#include "FreeRTOS.h"
#include "ev_hc32f4a0_lqfp176_bsp.h"
#include <string.h>
#include <stdlib.h>

#include "parameter.h"
#include "initandset.h"
#include "bcmu_tcp.h"

typedef struct{
    uint32_t data_refresh_task_heartbeat;
    struct tskTaskControlBlock * start_task_handler;
    struct tskTaskControlBlock * data_refresh_task_handler;
    struct tskTaskControlBlock * sockets_accept_server_handler;
}task_st;
extern task_st task;

enum
{
    Time_data_refresh_task = 0,
};

extern portTickType xLastWakeTime[];

#endif /* __MAIN_H__ */
