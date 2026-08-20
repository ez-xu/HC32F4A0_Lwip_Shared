/* ----------------------------------------------------------------------
 * $Date:        5. February 2013
 * $Revision:    V1.02
 *
 * Project:      CMSIS-RTOS API
 * Title:        cmsis_os.c source file
 *
 * Version 0.02
 *    Initial Proposal Phase
 * Version 0.03
 *    osKernelStart added, optional feature: main started as thread
 *    osSemaphores have standard behavior
 *    osTimerCreate does not start the timer, added osTimerStart
 *    osThreadPass is renamed to osThreadYield
 * Version 1.01
 *    Support for C++ interface
 *     - const attribute removed from the osXxxxDef_t typedef's
 *     - const attribute added to the osXxxxDef macros
 *    Added: osTimerDelete, osMutexDelete, osSemaphoreDelete
 *    Added: osKernelInitialize
 * Version 1.02
 *    Control functions for short timeouts in microsecond resolution:
 *    Added: osKernelSysTick, osKernelSysTickFrequency, osKernelSysTickMicroSec
 *    Removed: osSignalGet
 *----------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2017 ARM LIMITED
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *---------------------------------------------------------------------------*/

#include "cmsis_os.h"
#include "hc32_ll_def.h"
#include <string.h>
#if defined ( __GNUC__ ) && !defined (__CC_ARM) /*!< GNU Compiler */
#include "cmsis_gcc.h"
#elif defined (__CC_ARM)                /*!< ARM Compiler */
#include "cmsis_armcc.h"
#endif

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
extern void xPortSysTickHandler(void);

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
/* Convert from CMSIS priority to FreeRTOS priority */
static UBaseType_t ConvertFreeRtosPriority(osPriority priority)
{
    UBaseType_t uxPriority = tskIDLE_PRIORITY;

    if (priority != osPriorityError) {
        uxPriority += (priority - osPriorityIdle);
    }
    return uxPriority;
}

#if (INCLUDE_uxTaskPriorityGet == 1)
/* Convert from FreeRTOS priority to CMSIS priority */
static osPriority ConvertCmsisPriority(UBaseType_t uxPriority)
{
    osPriority priority = osPriorityError;

    uxPriority -= tskIDLE_PRIORITY;
    if (uxPriority <= (osPriorityRealtime - osPriorityIdle)) {
        priority = (osPriority)((int)osPriorityIdle + (int)(uxPriority));
    }
    return priority;
}
#endif

/* Determine whether the core is in handler mode. */
static uint32_t CoreInHandlerMode(void)
{
    return (__get_IPSR() != 0U);
}

/*********************** Kernel Control Functions *****************************/
/**
 * @brief  Initialize the RTOS Kernel for creating objects.
 * @retval status code that indicates the execution status of the function.
 */
osStatus osKernelInitialize(void);

/**
 * @brief  Start the RTOS Kernel.
 * @retval status code that indicates the execution status of the function.
 */
osStatus osKernelStart(void)
{
    vTaskStartScheduler();
    return osOK;
}

/**
 * @brief  Check if the RTOS kernel is already started.
 * @param  None
 * @retval 0: RTOS is not started, 1: RTOS is started.
 *         -1: this feature is disabled in FreeRTOSConfig.h.
 */
int32_t osKernelRunning(void)
{
#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return 0;
    } else {
        return 1;
    }
#else
    return -1;
#endif
}

#if (defined (osFeature_SysTick)  &&  (osFeature_SysTick != 0))     // System Timer available
/**
 * @brief  Get the RTOS kernel system timer counter.
 * @param  None
 * @retval uint32_t                     RTOS kernel system timer as 32-bit value.
 */
uint32_t osKernelSysTick(void)
{
    if (CoreInHandlerMode()) {
        return xTaskGetTickCountFromISR();
    } else {
        return xTaskGetTickCount();
    }
}
#endif    // System Timer available

/**************************** Thread Management *******************************/
/**
 * @brief  Create a thread and add it to Active Threads and set it to state READY.
 * @param  thread_def                   thread definition referenced with \ref osThread.
 * @param  argument                     pointer that is passed to the thread function as start argument.
 * @retval osThreadId                   thread ID for reference by other functions or NULL in case of error.
 */
osThreadId osThreadCreate(const osThreadDef_t *thread_def, void *argument)
{
    TaskHandle_t handle;

#if( configSUPPORT_STATIC_ALLOCATION == 1 ) &&  ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    if ((thread_def->stackbuffer != NULL) && (thread_def->controlblock != NULL)) {
        handle = xTaskCreateStatic((TaskFunction_t)thread_def->pthread, thread_def->name, thread_def->stacksize,
                                   argument, ConvertFreeRtosPriority(thread_def->tpriority),
                                   thread_def->stackbuffer, thread_def->controlblock);
    } else {
        if (xTaskCreate((TaskFunction_t)thread_def->pthread, thread_def->name, thread_def->stacksize,
                        argument, ConvertFreeRtosPriority(thread_def->tpriority), &handle) != pdPASS)  {
            return NULL;
        }
    }
#elif( configSUPPORT_STATIC_ALLOCATION == 1 )
    handle = xTaskCreateStatic((TaskFunction_t)thread_def->pthread, thread_def->name, thread_def->stacksize,
                               argument, ConvertFreeRtosPriority(thread_def->tpriority),
                               thread_def->stackbuffer, thread_def->controlblock);
#else
    if (xTaskCreate((TaskFunction_t)thread_def->pthread, thread_def->name, thread_def->stacksize,
                    argument, ConvertFreeRtosPriority(thread_def->tpriority), &handle) != pdPASS)  {
        return NULL;
    }
#endif

    return handle;
}

/**
 * @brief  Return the thread ID of the current running thread.
 * @retval osThreadId                   thread ID for reference by other functions or NULL in case of error.
 */
osThreadId osThreadGetId(void)
{
#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )
    return xTaskGetCurrentTaskHandle();
#else
    return NULL;
#endif
}

/**
 * @brief  Terminate execution of a thread and remove it from Active Threads.
 * @param  thread_id                    thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osThreadTerminate(osThreadId thread_id)
{
#if (INCLUDE_vTaskDelete == 1)
    vTaskDelete(thread_id);
    return osOK;
#else
    return osErrorOS;
#endif
}

/**
 * @brief  Pass control to next thread that is in state \b READY.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osThreadYield(void)
{
    taskYIELD();
    return osOK;
}

/**
 * @brief  Change priority of an active thread.
 * @param  thread_id                    thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
 * @param  priority                     new priority value for the thread function.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osThreadSetPriority(osThreadId thread_id, osPriority priority)
{
#if (INCLUDE_vTaskPrioritySet == 1)
    vTaskPrioritySet(thread_id, ConvertFreeRtosPriority(priority));
    return osOK;
#else
    return osErrorOS;
#endif
}

/**
 * @brief   Get current priority of an active thread.
 * @param   thread_id                   thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
 * @retval  osPriority                  current priority value of the thread function.
 */
osPriority osThreadGetPriority(osThreadId thread_id)
{
#if (INCLUDE_uxTaskPriorityGet == 1)
    if (CoreInHandlerMode()) {
        return ConvertCmsisPriority(uxTaskPriorityGetFromISR(thread_id));
    } else {
        return ConvertCmsisPriority(uxTaskPriorityGet(thread_id));
    }
#else
    return osPriorityError;
#endif
}

/************************** Generic Wait Functions ****************************/
/**
 * @brief  Wait for Timeout (Time Delay).
 * @param  millisec                     time delay value
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osDelay(uint32_t millisec)
{
#if ( INCLUDE_vTaskDelay == 1 )
    TickType_t ticks = millisec / portTICK_PERIOD_MS;
    vTaskDelay((ticks != 0) ? ticks : 1);
    return osOK;
#else
    (void)millisec;
    return osErrorResource;
#endif
}

/************************ Timer Management Functions **************************/
/**
 * @brief  Create a timer.
 * @param  timer_def                    timer object referenced with \ref osTimer.
 * @param  type                         osTimerOnce for one-shot or osTimerPeriodic for periodic behavior.
 * @param  argument                     argument to the timer call back function.
 * @retval osTimerId                    timer ID for reference by other functions or NULL in case of error.
 */
osTimerId osTimerCreate(const osTimerDef_t *timer_def, os_timer_type type, void *argument)
{
#if (configUSE_TIMERS == 1)
#if( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    if (timer_def->controlblock != NULL) {
        return xTimerCreateStatic((char *)"", 1, (type == osTimerPeriodic) ? pdTRUE : pdFALSE, (void *)argument,
                                  (TimerCallbackFunction_t)timer_def->ptimer, (StaticTimer_t *)timer_def->controlblock);
    } else {
        return xTimerCreate((char *)"", 1, (type == osTimerPeriodic) ? pdTRUE : pdFALSE,
                            (void *)argument, (TimerCallbackFunction_t)timer_def->ptimer);
    }
#elif( configSUPPORT_STATIC_ALLOCATION == 1 )
    return xTimerCreateStatic((char *)"", 1, (type == osTimerPeriodic) ? pdTRUE : pdFALSE, (void *)argument,
                              (TimerCallbackFunction_t)timer_def->ptimer, (StaticTimer_t *)timer_def->controlblock);
#else
    return xTimerCreate((char *)"", 1, (type == osTimerPeriodic) ? pdTRUE : pdFALSE,
                        (void *)argument, (TimerCallbackFunction_t)timer_def->ptimer);
#endif
#else
    return NULL;
#endif
}

/**
 * @brief  Start or restart a timer.
 * @param  timer_id                     timer ID obtained by \ref osTimerCreate.
 * @param  millisec                     time delay value of the timer.
 * @retval osStatus                     status code that indicates the execution status of the function
 */
osStatus osTimerStart(osTimerId timer_id, uint32_t millisec)
{
    osStatus result = osOK;

#if (configUSE_TIMERS == 1)
    portBASE_TYPE taskWoken = pdFALSE;
    TickType_t ticks = millisec / portTICK_PERIOD_MS;

    if (ticks == 0) {
        ticks = 1;
    }
    if (CoreInHandlerMode()) {
        if (xTimerChangePeriodFromISR(timer_id, ticks, &taskWoken) != pdPASS) {
            result = osErrorOS;
        } else {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        if (xTimerChangePeriod(timer_id, ticks, 0) != pdPASS) {
            result = osErrorOS;
        }
    }
#else
    result = osErrorOS;
#endif
    return result;
}

/**
 * @brief  Stop a timer.
 * @param  timer_id                     timer ID obtained by \ref osTimerCreate.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osTimerStop(osTimerId timer_id)
{
    osStatus result = osOK;

#if (configUSE_TIMERS == 1)
    portBASE_TYPE taskWoken = pdFALSE;
    if (CoreInHandlerMode()) {
        if (xTimerStopFromISR(timer_id, &taskWoken) != pdPASS) {
            return osErrorOS;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else {
        if (xTimerStop(timer_id, 0) != pdPASS) {
            result = osErrorOS;
        }
    }
#else
    result = osErrorOS;
#endif
    return result;
}

/**
 * @brief  Delete a timer.
 * @param  timer_id                     timer ID obtained by \ref osTimerCreate
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osTimerDelete(osTimerId timer_id)
{
    osStatus result = osOK;

#if (configUSE_TIMERS == 1)
    if (CoreInHandlerMode()) {
        return osErrorISR;
    } else {
        if ((xTimerDelete(timer_id, osWaitForever)) != pdPASS) {
            result = osErrorOS;
        }
    }
#else
    result = osErrorOS;
#endif
    return result;
}

/***************************** Signal Management ******************************/
/**
 * @brief  Set the specified Signal Flags of an active thread.
 * @param  thread_id                    thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
 * @param  signals                      specifies the signal flags of the thread that should be set.
 * @retval int32_t                      previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
 */
int32_t osSignalSet(osThreadId thread_id, int32_t signals)
{
#if( configUSE_TASK_NOTIFICATIONS == 1 )
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t ulPreviousNotificationValue = 0;

    if (CoreInHandlerMode()) {
        if (xTaskGenericNotifyFromISR(thread_id, 0, (uint32_t)signals, eSetBits, &ulPreviousNotificationValue, &xHigherPriorityTaskWoken) != pdPASS) {
            return 0x80000000;
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else if (xTaskGenericNotify(thread_id, 0, (uint32_t)signals, eSetBits, &ulPreviousNotificationValue) != pdPASS) {
        return 0x80000000;
    }
    return ulPreviousNotificationValue;
#else
    (void)thread_id;
    (void)signals;
    return 0x80000000;  /* Task Notification not supported */
#endif
}

/**
 * @brief  Clear the specified Signal Flags of an active thread.
 * @param  thread_id                    thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
 * @param  signals                      specifies the signal flags of the thread that shall be cleared.
 * @retval int32_t                      previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters or call from ISR.
 */
int32_t osSignalClear(osThreadId thread_id, int32_t signals);

/**
 * @brief  Wait for one or more Signal Flags to become signaled for the current \b RUNNING thread.
 * @param  signals                      wait until all specified signal flags set or 0 for any single signal flag.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval osEvent                      event flag information or error code.
 */
osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
    osEvent ret;

#if( configUSE_TASK_NOTIFICATIONS == 1 )
    TickType_t ticks;

    ret.value.signals = 0;
    ticks = 0;
    if (millisec == osWaitForever) {
        ticks = portMAX_DELAY;
    } else if (millisec != 0) {
        ticks = millisec / portTICK_PERIOD_MS;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    if (CoreInHandlerMode()) {
        ret.status = osErrorISR;
    } else {
        if (xTaskNotifyWait(0, (uint32_t)signals, (uint32_t *)&ret.value.signals, ticks) != pdTRUE) {
            if (ticks == 0) {
                ret.status = osOK;
            } else {
                ret.status = osEventTimeout;
            }
        } else if (ret.value.signals < 0) {
            ret.status = osErrorValue;
        } else {
            ret.status = osEventSignal;
        }
    }
#else
    (void)signals;
    (void)millisec;
    ret.status = osErrorOS; /* Task Notification not supported */
#endif

    return ret;
}

/****************************  Mutex Management *******************************/
/**
 * @brief  Create and Initialize a Mutex object.
 * @param  mutex_def                    mutex definition referenced with \ref osMutex.
 * @retval osMutexId                    mutex ID for reference by other functions or NULL in case of error.
 */
osMutexId osMutexCreate(const osMutexDef_t *mutex_def)
{
#if ( configUSE_MUTEXES == 1)
#if( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    if (mutex_def->controlblock != NULL) {
        return xSemaphoreCreateMutexStatic(mutex_def->controlblock);
    } else {
        return xSemaphoreCreateMutex();
    }
#elif ( configSUPPORT_STATIC_ALLOCATION == 1 )
    return xSemaphoreCreateMutexStatic(mutex_def->controlblock);
#else
    return xSemaphoreCreateMutex();
#endif
#else
    return NULL;
#endif
}

/**
 * @brief  Wait until a Mutex becomes available.
 * @param  mutex_id                     mutex ID obtained by \ref osMutexCreate.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osMutexWait(osMutexId mutex_id, uint32_t millisec)
{
    TickType_t ticks = 0;
    portBASE_TYPE taskWoken = pdFALSE;

    if (millisec == osWaitForever) {
        ticks = portMAX_DELAY;
    } else if (millisec != 0) {
        ticks = millisec / portTICK_PERIOD_MS;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    if (CoreInHandlerMode()) {
        if (xSemaphoreTakeFromISR(mutex_id, &taskWoken) != pdTRUE) {
            return osErrorOS;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else if (xSemaphoreTake(mutex_id, ticks) != pdTRUE) {
        return osErrorOS;
    }

    return osOK;
}

/**
 * @brief  Release a Mutex that was obtained by \ref osMutexWait.
 * @param  mutex_id                     mutex ID obtained by \ref osMutexCreate.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osMutexRelease(osMutexId mutex_id)
{
    osStatus result = osOK;
    portBASE_TYPE taskWoken = pdFALSE;

    if (CoreInHandlerMode()) {
        if (xSemaphoreGiveFromISR(mutex_id, &taskWoken) != pdTRUE) {
            return osErrorOS;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else if (xSemaphoreGive(mutex_id) != pdTRUE) {
        result = osErrorOS;
    }

    return result;
}

/**
 * @brief  Delete a Mutex that was created by \ref osMutexCreate.
 * @param  mutex_id                     mutex ID obtained by \ref osMutexCreate.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osMutexDelete(osMutexId mutex_id)
{
    if (CoreInHandlerMode()) {
        return osErrorISR;
    }
    vQueueDelete(mutex_id);

    return osOK;
}

/********************  Semaphore Management Functions *************************/
#if (defined (osFeature_Semaphore)  &&  (osFeature_Semaphore != 0))
/**
 * @brief  Create and Initialize a Semaphore object used for managing resources.
 * @param  semaphore_def                semaphore definition referenced with \ref osSemaphore.
 * @param  count                        number of available resources.
 * @retval osSemaphoreId                semaphore ID for reference by other functions or NULL in case of error.
 */
osSemaphoreId osSemaphoreCreate(const osSemaphoreDef_t *semaphore_def, int32_t count)
{
#if( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    osSemaphoreId sem;

    if (semaphore_def->controlblock != NULL) {
        if (count == 1) {
            return xSemaphoreCreateBinaryStatic(semaphore_def->controlblock);
        } else {
#if (configUSE_COUNTING_SEMAPHORES == 1 )
            return xSemaphoreCreateCountingStatic(count, count, semaphore_def->controlblock);
#else
            return NULL;
#endif
        }
    } else {
        if (count == 1) {
            vSemaphoreCreateBinary(sem);
            return sem;
        } else {
#if (configUSE_COUNTING_SEMAPHORES == 1 )
            return xSemaphoreCreateCounting(count, count);
#else
            return NULL;
#endif
        }
    }
#elif ( configSUPPORT_STATIC_ALLOCATION == 1 )
    if (count == 1) {
        return xSemaphoreCreateBinaryStatic(semaphore_def->controlblock);
    } else {
#if (configUSE_COUNTING_SEMAPHORES == 1 )
        return xSemaphoreCreateCountingStatic(count, count, semaphore_def->controlblock);
#else
        return NULL;
#endif
    }
#else
    osSemaphoreId sem;

    if (count == 1) {
        vSemaphoreCreateBinary(sem);
        return sem;
    } else {
#if (configUSE_COUNTING_SEMAPHORES == 1 )
        return xSemaphoreCreateCounting(count, count);
#else
        return NULL;
#endif
    }
#endif
}

/**
 * @brief  Wait until a Semaphore token becomes available.
 * @param  semaphore_id                 semaphore object referenced with \ref osSemaphore.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval int32_t                      number of available tokens, or -1 in case of incorrect parameters.
 */
int32_t osSemaphoreWait(osSemaphoreId semaphore_id, uint32_t millisec)
{
    TickType_t ticks = 0;
    portBASE_TYPE taskWoken = pdFALSE;

    if (millisec == osWaitForever) {
        ticks = portMAX_DELAY;
    } else if (millisec != 0) {
        ticks = millisec / portTICK_PERIOD_MS;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    if (CoreInHandlerMode()) {
        if (xSemaphoreTakeFromISR(semaphore_id, &taskWoken) != pdTRUE) {
            return osErrorOS;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else if (xSemaphoreTake(semaphore_id, ticks) != pdTRUE) {
        return osErrorOS;
    }

    return osOK;
}

/**
 * @brief  Release a Semaphore token.
 * @param  semaphore_id                 semaphore object referenced with \ref osSemaphore.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osSemaphoreRelease(osSemaphoreId semaphore_id)
{
    osStatus result = osOK;
    portBASE_TYPE taskWoken = pdFALSE;

    if (CoreInHandlerMode()) {
        if (xSemaphoreGiveFromISR(semaphore_id, &taskWoken) != pdTRUE) {
            return osErrorOS;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else {
        if (xSemaphoreGive(semaphore_id) != pdTRUE) {
            result = osErrorOS;
        }
    }

    return result;
}

/**
 * @brief  Delete a Semaphore that was created by \ref osSemaphoreCreate.
 * @param  semaphore_id                 semaphore object referenced with \ref osSemaphore.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osSemaphoreDelete(osSemaphoreId semaphore_id)
{
    if (CoreInHandlerMode()) {
        return osErrorISR;
    }
    vSemaphoreDelete(semaphore_id);

    return osOK;
}
#endif    /* Use Semaphores */

/********************* Memory Pool Management Functions ***********************/
#if (defined (osFeature_Pool)  &&  (osFeature_Pool != 0))

typedef struct os_pool_cb {
    uint32_t pool_sz;
    uint32_t item_sz;
    void     *pool;
    uint8_t  *mark;
    uint32_t index;
} os_pool_cb_t;

/**
 * @brief  Create and Initialize a memory pool.
 * @param  pool_def                     memory pool definition referenced with \ref osPool.
 * @retval osPoolId                     memory pool ID for reference by other functions or NULL in case of error.
 */
osPoolId osPoolCreate(const osPoolDef_t *pool_def)
{
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    uint32_t i;
    osPoolId poolId;
    uint32_t itemSize;

    poolId = pvPortMalloc(sizeof(os_pool_cb_t));
    if (poolId != NULL) {
        itemSize = 4 * ((pool_def->item_sz + 3) / 4);
        poolId->pool_sz = pool_def->pool_sz;
        poolId->item_sz = itemSize;
        poolId->index   = 0;
        /* Allocate memory for the pool mark */
        poolId->mark = pvPortMalloc(pool_def->pool_sz);
        if (poolId->mark != NULL) {
            /* allocate memory for the pool */
            poolId->pool = pvPortMalloc(pool_def->pool_sz * itemSize);
            if (poolId->pool != NULL) {
                for (i = 0; i < pool_def->pool_sz; i++) {
                    poolId->mark[i] = 0;
                }
            } else {
                vPortFree(poolId->mark);
                vPortFree(poolId);
                poolId = NULL;
            }
        } else {
            vPortFree(poolId);
            poolId = NULL;
        }
    }
    return poolId;
#else
    return NULL;
#endif
}

/**
 * @brief  Allocate a memory block from a memory pool.
 * @param  pool_id                      memory pool ID obtain referenced with \ref osPoolCreate.
 * @retval void*                        address of the allocated memory block or NULL in case of no memory available.
 */
void *osPoolAlloc(osPoolId pool_id)
{
    int dummy = 0;
    void *pItem = NULL;
    uint32_t i;
    uint32_t index;

    if (CoreInHandlerMode()) {
        dummy = portSET_INTERRUPT_MASK_FROM_ISR();
    } else {
        vPortEnterCritical();
    }
    for (i = 0; i < pool_id->pool_sz; i++) {
        index = pool_id->index + i;
        if (index >= pool_id->pool_sz) {
            index = 0;
        }
        if (pool_id->mark[index] == 0) {
            pool_id->mark[index] = 1;
            pItem = (void *)((uint32_t)(pool_id->pool) + (index * pool_id->item_sz));
            pool_id->index = index;
            break;
        }
    }
    if (CoreInHandlerMode()) {
        portCLEAR_INTERRUPT_MASK_FROM_ISR(dummy);
    } else {
        vPortExitCritical();
    }

    return pItem;
}

/**
 * @brief  Allocate a memory block from a memory pool and set memory block to zero.
 * @param  pool_id                      memory pool ID obtain referenced with \ref osPoolCreate.
 * @retval void*                        address of the allocated memory block or NULL in case of no memory available.
 */
void *osPoolCAlloc(osPoolId pool_id)
{
    void *pItem;

    pItem = osPoolAlloc(pool_id);
    if (pItem != NULL) {
        memset(pItem, 0, sizeof(pool_id->pool_sz));
    }

    return pItem;
}

/**
 * @brief  Return an allocated memory block back to a specific memory pool.
 * @param  pool_id                      memory pool ID obtain referenced with \ref osPoolCreate.
 * @param  block                        address of the allocated memory block that is returned to the memory pool.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osPoolFree(osPoolId pool_id, void *block)
{
    uint32_t addr;
    uint32_t index;

    if ((pool_id == NULL) || (block == NULL) || (block < pool_id->pool)) {
        return osErrorParameter;
    }

    addr = (uint32_t)block - (uint32_t)(pool_id->pool);
    if ((addr % pool_id->item_sz) != 0) {
        return osErrorParameter;
    }
    index = addr / pool_id->item_sz;
    if (index >= pool_id->pool_sz) {
        return osErrorParameter;
    }
    pool_id->mark[index] = 0;

    return osOK;
}
#endif

/******************** Message Queue Management Functions **********************/
#if (defined (osFeature_MessageQ)  &&  (osFeature_MessageQ != 0))
/**
 * @brief  Create and Initialize a Message Queue.
 * @param  queue_def                    queue definition referenced with \ref osMessageQ.
 * @param  thread_id                    thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId) or NULL.
 * @retval osMessageQId                 message queue ID for reference by other functions or NULL in case of error.
 */
osMessageQId osMessageCreate(const osMessageQDef_t *queue_def, osThreadId thread_id)
{
    (void)thread_id;
#if( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    if ((queue_def->buffer != NULL) && (queue_def->controlblock != NULL)) {
        return xQueueCreateStatic(queue_def->queue_sz, queue_def->item_sz, queue_def->buffer, queue_def->controlblock);
    } else {
        return xQueueCreate(queue_def->queue_sz, queue_def->item_sz);
    }
#elif ( configSUPPORT_STATIC_ALLOCATION == 1 )
    return xQueueCreateStatic(queue_def->queue_sz, queue_def->item_sz, queue_def->buffer, queue_def->controlblock);
#else
    return xQueueCreate(queue_def->queue_sz, queue_def->item_sz);
#endif
}

/**
 * @brief  Put a Message to a Queue.
 * @param  queue_id                     message queue ID obtained with \ref osMessageCreate.
 * @param  info                         message information.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osMessagePut(osMessageQId queue_id, uint32_t info, uint32_t millisec)
{
    portBASE_TYPE taskWoken = pdFALSE;
    TickType_t ticks;

    ticks = millisec / portTICK_PERIOD_MS;
    if (ticks == 0) {
        ticks = 1;
    }
    if (CoreInHandlerMode()) {
        if (xQueueSendFromISR(queue_id, &info, &taskWoken) != pdTRUE) {
            return osErrorOS;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else {
        if (xQueueSend(queue_id, &info, ticks) != pdTRUE) {
            return osErrorOS;
        }
    }

    return osOK;
}

/**
 * @brief  Get a Message or Wait for a Message from a Queue.
 * @param  queue_id                     message queue ID obtained with \ref osMessageCreate.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval osEvent                      event information that includes status code.
 */
osEvent osMessageGet(osMessageQId queue_id, uint32_t millisec)
{
    portBASE_TYPE taskWoken = pdFALSE;
    TickType_t ticks = 0;
    osEvent event;

    event.def.message_id = queue_id;
    event.value.v = 0;
    if (queue_id == NULL) {
        event.status = osErrorParameter;
        return event;
    }
    if (millisec == osWaitForever) {
        ticks = portMAX_DELAY;
    } else if (millisec != 0) {
        ticks = millisec / portTICK_PERIOD_MS;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    if (CoreInHandlerMode()) {
        if (xQueueReceiveFromISR(queue_id, &event.value.v, &taskWoken) == pdTRUE) {
            event.status = osEventMessage;
        } else {
            event.status = osOK;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else {
        if (xQueueReceive(queue_id, &event.value.v, ticks) == pdTRUE) {
            event.status = osEventMessage;
        } else {
            event.status = (ticks == 0) ? osOK : osEventTimeout;
        }
    }

    return event;
}
#endif  /* Use Message Queues */

/********************* Mail Queue Management Functions ************************/
#if (defined (osFeature_MailQ)  &&  (osFeature_MailQ != 0))  /* Use Mail Queues */

typedef struct os_mailQ_cb {
    const osMailQDef_t *queue_def;
    QueueHandle_t       handle;
    osPoolId            pool;
} os_mailQ_cb_t;

/**
 * @brief  Create and Initialize mail queue.
 * @param  queue_def                    reference to the mail queue definition obtain with \ref osMailQ
 * @param  thread_id                    thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId) or NULL.
 * @retval osMailQId                    mail queue ID for reference by other functions or NULL in case of error.
 */
osMailQId osMailCreate(const osMailQDef_t *queue_def, osThreadId thread_id)
{
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    osMailQId mailQId;
    osPoolDef_t pool_def = {queue_def->queue_sz, queue_def->item_sz, NULL};

    (void)thread_id;
    /* Create a mail queue control block */
    mailQId = pvPortMalloc(sizeof(struct os_mailQ_cb));
    if (mailQId == NULL) {
        return NULL;
    }
    /* Create a queue  */
    mailQId->queue_def = queue_def;
    mailQId->handle    = xQueueCreate(queue_def->queue_sz, sizeof(void *));
    if (mailQId->handle == NULL) {
        vPortFree(mailQId);
        return NULL;
    }
    /* Create a mail pool */
    mailQId->pool = osPoolCreate(&pool_def);
    if (mailQId->pool == NULL) {
        vQueueDelete(mailQId->handle);
        vPortFree(mailQId);
        return NULL;
    }
    return mailQId;
#else
    return NULL;
#endif
}

/**
 * @brief  Allocate a memory block from a mail.
 * @param  queue_id                     mail queue ID obtained with \ref osMailCreate.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval void*                        pointer to memory block that can be filled with mail or NULL in case error.
 */
void *osMailAlloc(osMailQId queue_id, uint32_t millisec)
{
    (void)millisec;
    void *pItem;

    if (queue_id == NULL) {
        return NULL;
    }
    pItem = osPoolAlloc(queue_id->pool);

    return pItem;
}

/**
 * @brief  Allocate a memory block from a mail and set memory block to zero.
 * @param  queue_id                     mail queue ID obtained with \ref osMailCreate.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval void*                        pointer to memory block that can be filled with mail or NULL in case error.
 */
void *osMailCAlloc(osMailQId queue_id, uint32_t millisec)
{
    void *pItem;

    pItem = osMailAlloc(queue_id, millisec);
    if (pItem != NULL) {
        memset(pItem, 0, sizeof(queue_id->queue_def->item_sz));
    }

    return pItem;
}

/**
 * @brief  Put a mail to a queue.
 * @param  queue_id                     mail queue ID obtained with \ref osMailCreate.
 * @param  mail                         memory block previously allocated with \ref osMailAlloc or \ref osMailCAlloc.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osMailPut(osMailQId queue_id, void *mail)
{
    portBASE_TYPE taskWoken = pdFALSE;

    if (queue_id == NULL) {
        return osErrorParameter;
    }
    if (CoreInHandlerMode()) {
        if (xQueueSendFromISR(queue_id->handle, &mail, &taskWoken) != pdTRUE) {
            return osErrorOS;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else {
        if (xQueueSend(queue_id->handle, &mail, 0) != pdTRUE) {
            return osErrorOS;
        }
    }

    return osOK;
}

/**
 * @brief  Get a mail from a queue.
 * @param  queue_id                     mail queue ID obtained with \ref osMailCreate.
 * @param  millisec                     timeout value or 0 in case of no time-out
 * @retval osEvent                      event that contains mail information or error code.
 */
osEvent osMailGet(osMailQId queue_id, uint32_t millisec)
{
    portBASE_TYPE taskWoken = pdFALSE;
    TickType_t ticks = 0;
    osEvent event;

    event.def.mail_id = queue_id;
    if (queue_id == NULL) {
        event.status = osErrorParameter;
        return event;
    }

    if (millisec == osWaitForever) {
        ticks = portMAX_DELAY;
    } else if (millisec != 0) {
        ticks = millisec / portTICK_PERIOD_MS;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    if (CoreInHandlerMode()) {
        if (xQueueReceiveFromISR(queue_id->handle, &event.value.p, &taskWoken) == pdTRUE) {
            event.status = osEventMail;
        } else {
            event.status = osOK;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else {
        if (xQueueReceive(queue_id->handle, &event.value.p, ticks) == pdTRUE) {
            event.status = osEventMail;
        } else {
            event.status = (ticks == 0) ? osOK : osEventTimeout;
        }
    }

    return event;
}

/**
 * @brief  Free a memory block from a mail.
 * @param  queue_id                     mail queue ID obtained with \ref osMailCreate.
 * @param  mail                         pointer to the memory block that was obtained with \ref osMailGet.
 * @retval osStatus                     status code that indicates the execution status of the function.
 */
osStatus osMailFree(osMailQId queue_id, void *mail)
{
    if (queue_id == NULL) {
        return osErrorParameter;
    }
    return osPoolFree(queue_id->pool, mail);
}
#endif  /* Use Mail Queues */

/********************** Additional specific APIs to Free RTOS *****************/
/**
 * @brief  Systick Handles.
 * @param  None
 * @retval None
 */
void osSystickHandler(void)
{
#if (INCLUDE_xTaskGetSchedulerState  == 1 )
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
#endif
        xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState  == 1 )
    }
#endif
}

#if ( INCLUDE_eTaskGetState == 1 )
/**
 * @brief  Obtain the state of any thread.
 * @param  thread_id                    thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId).
 * @retval osThreadState                An @ref osThreadState enumeration value.
 */
osThreadState osThreadGetState(osThreadId thread_id)
{
    eTaskState threadState;
    osThreadState result;

    threadState = eTaskGetState(thread_id);
    switch (threadState) {
        case eRunning:
            result = osThreadRunning;
            break;
        case eReady:
            result = osThreadReady;
            break;
        case eBlocked:
            result = osThreadBlocked;
            break;
        case eSuspended:
            result = osThreadSuspended;
            break;
        case eDeleted:
            result = osThreadDeleted;
            break;
        default:
            result = osThreadInvalid;
    }

    return result;
}
#endif /* INCLUDE_eTaskGetState */

/**
 * @brief  Suspend execution of a thread.
 * @param  thread_id                    thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId).
 *                                      Passing a NULL handle will cause the calling task to be suspended.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osThreadSuspend(osThreadId thread_id)
{
#if (INCLUDE_vTaskSuspend == 1)
    vTaskSuspend(thread_id);
    return osOK;
#else
    return osErrorResource;
#endif
}

/**
 * @brief  Resume execution of a suspended thread.
 * @param  thread_id                    thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osThreadResume(osThreadId thread_id)
{
#if (INCLUDE_vTaskSuspend == 1)
    if (CoreInHandlerMode()) {
        if (xTaskResumeFromISR(thread_id) == pdTRUE) {
            portYIELD_FROM_ISR(pdTRUE);
        }
    } else {
        vTaskResume(thread_id);
    }
    return osOK;
#else
    return osErrorResource;
#endif
}

/**
 * @brief  Suspend execution of a all active threads.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osThreadSuspendAll(void)
{
    vTaskSuspendAll();
    return osOK;
}

/**
 * @brief  Resume execution of a all suspended threads.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osThreadResumeAll(void)
{
    if (xTaskResumeAll() == pdTRUE)
        return osOK;
    return osErrorOS;
}

/**
 * @brief  Lists all the current tasks, along with their current state and stack usage high water mark.
 * @param  buffer                       A buffer into which the above mentioned details will be written.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osThreadList(uint8_t *buffer)
{
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 ) )
    vTaskList((char *)buffer);
#endif
    return osOK;
}

/**
 * @brief  Delay a task until a specified time.
 * @param  previousWakeTime             Pointer to a variable that holds the time at which the task was last unblocked.
 * @param  millisec                     time delay value.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osDelayUntil(uint32_t *previousWakeTime, uint32_t millisec)
{
#if ( INCLUDE_vTaskDelayUntil == 1 )
    TickType_t ticks = (millisec / portTICK_PERIOD_MS);
    vTaskDelayUntil((TickType_t *) previousWakeTime, (ticks != 0) ? ticks : 1);
    return osOK;
#else
    (void)millisec;
    (void)previousWakeTime;
    return osErrorResource;
#endif
}

/**
 * @brief  Abort the delay for a specific thread.
 * @param  thread_id                    thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osAbortDelay(osThreadId thread_id)
{
#if( INCLUDE_xTaskAbortDelay == 1 )
    xTaskAbortDelay(thread_id);
    return osOK;
#else
    (void)thread_id;
    return osErrorResource;
#endif
}

/**
 * @brief  Returns the current count value of a counting semaphore.
 * @param  semaphore_id                 semaphore object referenced with \ref osSemaphoreCreate.
 * @retval count value.
 */
uint32_t osSemaphoreGetCount(osSemaphoreId semaphore_id)
{
    return uxSemaphoreGetCount(semaphore_id);
}

/**
 * @brief  Create and Initialize a Recursive Mutex.
 * @param  mutex_def                    mutex definition referenced with \ref osMutex.
 * @retval osMutexId                    mutex ID or NULL in case of error.
 */
osMutexId osRecursiveMutexCreate(const osMutexDef_t *mutex_def)
{
#if (configUSE_RECURSIVE_MUTEXES == 1)
#if( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    if (mutex_def->controlblock != NULL) {
        return xSemaphoreCreateRecursiveMutexStatic(mutex_def->controlblock);
    } else {
        return xSemaphoreCreateRecursiveMutex();
    }
#elif ( configSUPPORT_STATIC_ALLOCATION == 1 )
    return xSemaphoreCreateRecursiveMutexStatic(mutex_def->controlblock);
#else
    return xSemaphoreCreateRecursiveMutex();
#endif
#else
    return NULL;
#endif
}

/**
 * @brief  Release a Recursive Mutex.
 * @param  mutex_id                     mutex ID obtained by \ref osRecursiveMutexCreate.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osRecursiveMutexRelease(osMutexId mutex_id)
{
#if (configUSE_RECURSIVE_MUTEXES == 1)
    osStatus result = osOK;

    if (xSemaphoreGiveRecursive(mutex_id) != pdTRUE) {
        result = osErrorOS;
    }
    return result;
#else
    return osErrorResource;
#endif
}

/**
 * @brief  Wait a Recursive Mutex.
 * @param  mutex_id                     mutex ID obtained by \ref osRecursiveMutexCreate.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osRecursiveMutexWait(osMutexId mutex_id, uint32_t millisec)
{
#if (configUSE_RECURSIVE_MUTEXES == 1)
    TickType_t ticks = 0;

    if (millisec == osWaitForever) {
        ticks = portMAX_DELAY;
    } else if (millisec != 0) {
        ticks = millisec / portTICK_PERIOD_MS;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    if (xSemaphoreTakeRecursive(mutex_id, ticks) != pdTRUE) {
        return osErrorOS;
    }
    return osOK;
#else
    return osErrorResource;
#endif
}

/**
 * @brief  Receive an item from a queue without removing the item from the queue.
 * @param  queue_id                     message queue ID obtained with \ref osMessageCreate.
 * @param  millisec                     timeout value or 0 in case of no time-out.
 * @retval osEvent                      event information that includes status code.
 */
osEvent osMessagePeek(osMessageQId queue_id, uint32_t millisec)
{
    TickType_t ticks = 0;
    osEvent event;

    event.def.message_id = queue_id;
    if (queue_id == NULL) {
        event.status = osErrorParameter;
        return event;
    }

    if (millisec == osWaitForever) {
        ticks = portMAX_DELAY;
    } else if (millisec != 0) {
        ticks = millisec / portTICK_PERIOD_MS;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    if (xQueuePeek(queue_id, &event.value.v, ticks) == pdTRUE) {
        event.status = osEventMessage;
    } else {
        event.status = (ticks == 0) ? osOK : osEventTimeout;
    }

    return event;
}

/**
 * @brief  Get the number of messaged stored in a queue.
 * @param  queue_id                     message queue ID obtained with \ref osMessageCreate.
 * @retval uint32_t                     number of messages stored in a queue.
 */
uint32_t osMessageWaiting(osMessageQId queue_id)
{
    if (CoreInHandlerMode()) {
        return uxQueueMessagesWaitingFromISR(queue_id);
    } else {
        return uxQueueMessagesWaiting(queue_id);
    }
}

/**
 * @brief  Get the available space in a message queue.
 * @param  queue_id                     message queue ID obtained with \ref osMessageCreate.
 * @retval uint32_t                     available space in a message queue.
 */
uint32_t osMessageAvailableSpace(osMessageQId queue_id)
{
    return uxQueueSpacesAvailable(queue_id);
}

/**
 * @brief  Delete a Message Queue.
 * @param  queue_id                     message queue ID obtained with \ref osMessageCreate.
 * @retval osStatus                     An @ref osStatus enumeration value.
 */
osStatus osMessageDelete(osMessageQId queue_id)
{
    if (CoreInHandlerMode()) {
        return osErrorISR;
    }
    vQueueDelete(queue_id);

    return osOK;
}
