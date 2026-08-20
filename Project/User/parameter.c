#include "parameter.h"
#include "hc32_ll_eth.h"

volatile SysParaVoltiage_st SysParaVoltiage;
SysParaKeep_st SysParaKeep;

void initValue(void)
{
    /* Derive MAC address bytes 2..5 from IP configuration before lwIP init */
    ETH_MAC_ADDR2 = SysParaKeep.ipaddr[0];
    ETH_MAC_ADDR3 = SysParaKeep.ipaddr[1];
    ETH_MAC_ADDR4 = SysParaKeep.ipaddr[2];
    ETH_MAC_ADDR5 = SysParaKeep.ipaddr[3] + SysParaKeep.ExtAddr;
}

void CheckPara(void)
{
    if(SysParaKeep.ExtAddr < 1 || SysParaKeep.ExtAddr > MAX_BCMU_ADDR)
    {
        SysParaKeep.ExtAddr = 1;
    }
    if(SysParaKeep.netmask[0] != 255)
    {
        SysParaKeep.ipaddr[0] = 192;
        SysParaKeep.ipaddr[1] = 168;
        SysParaKeep.ipaddr[2] = 1;
        SysParaKeep.ipaddr[3] = 20;
        SysParaKeep.netmask[0] = 255;
        SysParaKeep.netmask[1] = 255;
        SysParaKeep.netmask[2] = 255;
        SysParaKeep.netmask[3] = 0;
        SysParaKeep.gw[0] = 192;
        SysParaKeep.gw[1] = 168;
        SysParaKeep.gw[2] = 1;
        SysParaKeep.gw[3] = 1;
        SysParaKeep.TestModeEnable = 0;
    }
}
