# HC32F4A0 Ethernet / lwIP Driver Workspace

A trimmed, standalone workspace for the HC32F4A0 Ethernet (ETH + lwIP)
driver stack, shared for driver-level support and debugging.

## Scope

- Ethernet MAC driver glue: `Project/User/source/ethernetif.c`
- Netif configuration and link callbacks: `Project/User/source/app_ethernet.c`
- TCP server (port 502): `Project/User/bcmu_tcp.c` — accepts connections and
  **echoes received data back** for link-level verification (application
  protocol handling is not part of this workspace)
- System init (clock, RTC, GPIO, lwIP): `Project/User/initandset.c`
- Vendor LL drivers: `Project/Drivers/hc32_ll_driver/` (HC32F4A0)
- lwIP 2.1.2 sources: `Project/lwip/`
- FreeRTOS + CMSIS-RTOS v1: `Project/FreeRTOS/`
- Board support / peripheral drivers: `V5.1.5/drv/`

## Build

- Keil MDK 5, project file: `HC32F4A0_Lwip_V515.uvprojx`
- Target: `HC32F4A0_Lwip_V515`, output in `V5.1.5/Objects/`
- Command line: `UV4.exe -b HC32F4A0_Lwip_V515.uvprojx -j0`

## Runtime defaults

- Static IP: `192.168.1.21` (fixed when `TestModeEnable == 1`), port `502`
- MAC address bytes 2..5 derived from the IP configuration
- Link supervision: `EthernetIF_CheckLink()` runs every 100 ms in
  `data_refresh_task`

## Notes

- Business application code, parameter storage (FlashDB) and other
  peripheral stacks have been removed from this workspace; the Ethernet
  driver stack is fully functional and compiles cleanly.
- Modify freely for driver-level fixes; keep changes under `Project/User/`
  and `Project/Drivers/` where possible.
