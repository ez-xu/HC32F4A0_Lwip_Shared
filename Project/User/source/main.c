#include "main.h"
#include "ethernetif.h"
#include "app_ethernet.h"
#include "lwip/tcpip.h"
#include "bcmu_tcp.h"

task_st task;
portTickType xLastWakeTime[4];

void data_refresh_task(void *pvParameters)
{
    xLastWakeTime[Time_data_refresh_task] = xTaskGetTickCount();
    stc_eth_link_arg_t *LinkArg = (stc_eth_link_arg_t *)&EthLinkArg;

    while(1)
    {
        task.data_refresh_task_heartbeat++;
        EthernetIF_CheckLink(LinkArg->netif);
        vTaskDelayUntil(&xLastWakeTime[Time_data_refresh_task], pdMS_TO_TICKS(100));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    DDL_Printf(PRINT_LEVEL_ERROR, "Stack overflow in task: %s\n", pcTaskName);
    for(;;);
}

void start_task(void *pvParameters)
{
    /* Link check task */
    xTaskCreate(data_refresh_task, "data_refresh_task", 512, NULL, 2, &task.data_refresh_task_handler);
    /* TCP accept task */
    xTaskCreate(sockets_accept_server, "sockets_accept_server", 512, NULL, 2, &task.sockets_accept_server_handler);
    vTaskDelete(task.start_task_handler);
}

int main(void)
{
    InitSystem();
    xTaskCreate(start_task, "start_task", 256, NULL, 1, &task.start_task_handler);
    vTaskStartScheduler();
    return 0;
}

/* Static memory for IDLE task (configSUPPORT_STATIC_ALLOCATION) */
static StaticTask_t xIdleTaskTCB;
static StackType_t xIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* Static memory for Timer task */
static StaticTask_t xTimerTaskTCB;
static StackType_t xTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = xTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
