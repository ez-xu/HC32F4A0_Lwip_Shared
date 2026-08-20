Ethernet (lwIP) driver workspace

This is a trimmed, standalone workspace containing the Ethernet/lwIP
driver stack for the HC32F4A0. Business application code has been removed;
TCP port 502 accepts connections and echoes received data back for
link-level verification.

Key files:
  source/ethernetif.c   - Ethernet interface (driver glue, link check)
  source/app_ethernet.c - netif configuration and link callbacks
  bcmu_tcp.c            - TCP server (accept/connection tasks, echo)
  source/lwipopts.h     - lwIP configuration

Build with Keil MDK: open HC32F4A0_Lwip_V515.uvprojx, target
HC32F4A0_Lwip_V515. Default IP: 192.168.1.21 (ExtAddr default 1,
TestModeEnable=1), port 502.
