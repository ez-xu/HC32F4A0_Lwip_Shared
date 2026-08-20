#include "initandset.h"
#include "parameter.h"

#include "app_ethernet.h"
#include "ev_hc32f4a0_lqfp176_bsp.h"
#include "drv/drv_gpio.h"
#include "lwip/sys.h"
#include "drv/drv_rtc.h"


#include "stdbool.h"
/**
 * @brief  XTAL stop detect function init
 * @param  None
 * @retval None
 */
static void XtalStopDetctInit(void)
{
    stc_clock_xtalstd_init_t stcXtalstdInit;

    /* Enable xtal fault dectect and occur reset. */
    (void)CLK_XtalStdStructInit(&stcXtalstdInit);
    stcXtalstdInit.u8State = CLK_XTALSTD_ON;
//#if XTALSTOP_OPS == XTALSTOP_OPS_INT
//    stcXtalstdInit.u8Mode = CLK_XTALSTD_MD_INT;
//    stcXtalstdInit.u8Int = CLK_XTALSTD_INT_ON;
//    stcXtalstdInit.u8Reset = CLK_XTALSTD_RST_OFF;
//#elif XTALSTOP_OPS == XTALSTOP_OPS_RST
    stcXtalstdInit.u8Mode = CLK_XTALSTD_MD_RST;
    stcXtalstdInit.u8Int = CLK_XTALSTD_INT_OFF;
    stcXtalstdInit.u8Reset = CLK_XTALSTD_RST_ON;
//#endif
    (void)CLK_XtalStdInit(&stcXtalstdInit);
}

void InitSystem(void)
{
	taskENTER_CRITICAL();

	/* Peripheral registers write unprotected */
	LL_PERIPH_WE(LL_PERIPH_ALL);
	/* Configure BSP */
    BSP_CLK_Init();
	
	 /* Xtal stop detect initialize */ 
	XtalStopDetctInit();
	
	RTC_Config();
	
	drv_gpio_init();
	

	/* 根据Bootloader版本�动切�APP备份区地�和FlashDB分区�
	 * �份代码兼容两种布�（前提：APP代码体积<400K）：
	 * - B003: V2布局（�份�=0x100000，TSDB=0x1C8000�
	 * - B002: V1布局（�份�=0x06C000，TSDB=0x100000�
	 * 默�V2(B003)，�测到B002则切�为V1
	 * 注意：boot�0x00000410处存储版�字�串，由parameter.c的section属�固�
	 */

	// 初�化FlashDB参数系统(包含�动迁移和加载)
	CheckPara(); /* validate default network parameters (no flash load in this build) */
	initValue();
	/* 不知道为�么放到RTOS任务里面初�化会�致通�速度慢很� */
    /* Create tcp_ip stack thread */
    tcpip_init(NULL, NULL);
    /* Configure the Network interface */
    Netif_Config();
    /* Notify user about the network interface config */
    EthernetIF_NotifyConnStatus(&gnetif);
	
	taskEXIT_CRITICAL();
}
