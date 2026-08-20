/**
 *******************************************************************************
 * @file  freemodbus/modbus_tcp_rtos/source/app_ethernet.c
 * @brief Ethernet DHCP and Connect status module.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-07-03       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2023, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "app_ethernet.h"
#include "parameter.h"



/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#ifdef USE_DHCP
#define DHCP_MAX_TRIES                  (4U)
#endif

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
stc_eth_link_arg_t EthLinkArg;
struct netif gnetif;
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
#ifdef USE_DHCP
static __IO uint8_t u8DHCPState = DHCP_OFF;
#endif
static __IO uint8_t u8EthConnState = ETH_CONN_STATE_DISCONNECT;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
 
void Netif_Config(void)
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

#ifdef USE_DHCP
    ip_addr_set_zero_ip4(&ipaddr);
    ip_addr_set_zero_ip4(&netmask);
    ip_addr_set_zero_ip4(&gw);
#else
    IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
    IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
    if(SysParaKeep.TestModeEnable == 1)
    {
        IP_ADDR4(&ipaddr, 192, 168, 1, 21);
        IP_ADDR4(&netmask, 255, 255, 255, 0);
        IP_ADDR4(&gw, 192, 168, 1, 1);
    }
#endif /* USE_DHCP */
    /* Add the network interface */
    (void)netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
    /*  Registers the default network interface */
    netif_set_default(&gnetif);

    if (netif_is_link_up(&gnetif)) {
        /* When the netif is fully configured this function must be called */
        netif_set_up(&gnetif);
    } else {
        /* When the netif link is down this function must be called */
        netif_set_down(&gnetif);
    }
    /* Set the link callback function, this function is called on change of link status*/
    netif_set_link_callback(&gnetif, EthernetIF_LinkCallback);

    EthLinkArg.netif = &gnetif;
#ifdef ETH_INTERFACE_RMII
    /* create a binary semaphore */
    osSemaphoreDef(LinkSem);
    EthLinkSem = osSemaphoreCreate(osSemaphore(LinkSem), 1);
    EthLinkArg.sem   = EthLinkSem;
    /* Configure link interrupt IO for ETH RMII */
    ETH_LinkIntConfig();
#endif
    /* Create the Ethernet link thread */
    /*osThreadDef(EthLink, ETH_LinkThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 5);
    osThreadCreate(osThread(EthLink), &EthLinkArg);*/
}

/**
 * @brief  Notify network connect status of Ethernet.
 * @param  [in] netif                   Pointer to a struct netif structure
 * @retval None
 */
void EthernetIF_NotifyConnStatus(struct netif *netif)
{
    if (netif_is_up(netif)) {
#ifdef USE_DHCP
        u8DHCPState = DHCP_START;
#else
        u8EthConnState = ETH_CONN_STATE_CONNECTED;
        /* Turn On LED BLUE to indicate ETH and LwIP init success*/
        //BSP_LED_On(LED_BLUE);
#endif /* USE_DHCP */
    } else {
#ifdef USE_DHCP
        u8DHCPState = DHCP_LINK_DOWN;
#endif  /* USE_DHCP */
        u8EthConnState = ETH_CONN_STATE_DISCONNECT;
        /* Turn On LED RED to indicate ETH and LwIP init error */
        //BSP_LED_On(LED_RED);
    }
}

/**
 * @brief  Notify link status change.
 * @param  [in] netif                   Pointer to a struct netif structure
 * @retval None
 */
void EthernetIF_NotifyLinkChange(struct netif *netif)
{
#ifndef USE_DHCP
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
#endif

    if (netif_is_link_up(netif)) {
        //BSP_LED_Off(LED_RED);
        //BSP_LED_On(LED_BLUE);
        u8EthConnState = ETH_CONN_STATE_CONNECTED;
#ifdef USE_DHCP
        /* Update DHCP state machine */
        u8DHCPState = DHCP_START;
#else
        IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
        IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
        IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
        netif_set_addr(netif, &ipaddr, &netmask, &gw);
#endif /* USE_DHCP */
        /* When the netif is fully configured this function must be called. */
        netif_set_up(netif);
    } else {
        //BSP_LED_Off(LED_BLUE);
        //BSP_LED_On(LED_RED);
        u8EthConnState = ETH_CONN_STATE_DISCONNECT;
#ifdef USE_DHCP
        u8DHCPState = DHCP_LINK_DOWN;
#endif /* USE_DHCP */
        /*  When the netif link is down this function must be called. */
        netif_set_down(netif);
    }
}

#ifdef USE_DHCP
/**
 * @brief  Lwip DHCP thread.
 * @param  [in] argument                Pointer that is passed to the thread function as start argument.
 * @retval None
 */
void LwIP_DhcpThread(void const *argument)
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    struct dhcp *dhcp;
    struct netif *netif = (struct netif *)argument;

    for (;;) {
        switch (u8DHCPState) {
            case DHCP_START:
                ip_addr_set_zero_ip4(&netif->ip_addr);
                ip_addr_set_zero_ip4(&netif->netmask);
                ip_addr_set_zero_ip4(&netif->gw);
                u8DHCPState = DHCP_WAIT_ADDR;
                (void)dhcp_start(netif);
                break;
            case DHCP_WAIT_ADDR:
                if (0U != dhcp_supplied_address(netif)) {
                    u8DHCPState = DHCP_ADDR_ASSIGNED;
//                    BSP_LED_On(LED_BLUE);
                } else {
                    dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
                    /* DHCP timeout */
                    if (dhcp->tries > DHCP_MAX_TRIES) {
                        u8DHCPState = DHCP_TIMEOUT;
                        dhcp_stop(netif);
                        /* Static address used */
                        IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
                        IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
                        IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
                        netif_set_addr(netif, &ipaddr, &netmask, &gw);
//                        BSP_LED_On(LED_BLUE);
                    }
                }
                break;
            case DHCP_LINK_DOWN:
                /* Stop DHCP */
                dhcp_stop(netif);
                u8DHCPState = DHCP_OFF;
                break;
            default:
                break;
        }
        osDelay(100);
    }
}
#endif


/**
 * @brief  Get ethernet connect state.
 * @param  None
 * @retval uint8_t:
 *           - ETH_CONN_STATE_DISCONNECT: ethernet disconnect
 *           - ETH_CONN_STATE_CONNECTED: ethernet connected
 */
uint8_t EthernetIF_GetConnState(void)
{
    return u8EthConnState;
}

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
