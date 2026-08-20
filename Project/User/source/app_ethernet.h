/**
 *******************************************************************************
 * @file  freemodbus/modbus_tcp_rtos/source/app_ethernet.h
 * @brief Header for app_ethernet.c module.
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
#ifndef __APP_ETHERNET_H__
#define __APP_ETHERNET_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "ethernetif.h"
#include "lwip\tcpip.h"

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* Number of milliseconds when to check for link status from PHY */
#ifndef LINK_TIMER_INTERVAL
#define LINK_TIMER_INTERVAL                     (50U)
#endif

/* Enable DHCP, if disabled, Use static address */
//#define USE_DHCP

/* DHCP process states */
#define DHCP_OFF                                (0U)
#define DHCP_START                              (1U)
#define DHCP_WAIT_ADDR                          (2U)
#define DHCP_ADDR_ASSIGNED                      (3U)
#define DHCP_TIMEOUT                            (4U)
#define DHCP_LINK_DOWN                          (5U)

/* Static IP Address */
//#define IP_ADDR0                                (192U)
//#define IP_ADDR1                                (168U)
//#define IP_ADDR2                                (1U)
//#define IP_ADDR3                                (20U)

/* Static Netmask */
//#define NETMASK_ADDR0                           (255)
//#define NETMASK_ADDR1                           (255U)
//#define NETMASK_ADDR2                           (255U)
//#define NETMASK_ADDR3                           (0U)

/* Static Gateway Address*/
//#define GW_ADDR0                                (192U)
//#define GW_ADDR1                                (168U)
//#define GW_ADDR2                                (1U)
//#define GW_ADDR3                                (1U)
#define IP_ADDR0                                (SysParaKeep.ipaddr[0])
#define IP_ADDR1                                (SysParaKeep.ipaddr[1])
#define IP_ADDR2                                (SysParaKeep.ipaddr[2])
#define IP_ADDR3                                (SysParaKeep.ipaddr[3]+SysParaKeep.ExtAddr)
#define NETMASK_ADDR0                           (SysParaKeep.netmask[0])
#define NETMASK_ADDR1                           (SysParaKeep.netmask[1])
#define NETMASK_ADDR2                           (SysParaKeep.netmask[2])
#define NETMASK_ADDR3                           (SysParaKeep.netmask[3])
#define GW_ADDR0                                (SysParaKeep.gw[0])
#define GW_ADDR1                                (SysParaKeep.gw[1])
#define GW_ADDR2                                (SysParaKeep.gw[2])
#define GW_ADDR3                                (SysParaKeep.gw[3])

/* Ethernet connect state */
#define ETH_CONN_STATE_DISCONNECT               (0U)
#define ETH_CONN_STATE_CONNECTED                (1U)

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
extern stc_eth_link_arg_t EthLinkArg;
extern struct netif gnetif;
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
void Netif_Config(void);
void EthernetIF_NotifyConnStatus(struct netif *netif);
#ifdef USE_DHCP
void LwIP_DhcpThread(void const *argument);
#endif
uint8_t EthernetIF_GetConnState(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_ETHERNET_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
