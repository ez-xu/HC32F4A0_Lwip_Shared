#ifndef DRV_UART0_H__
#define DRV_UART0_H__

#include "lib/lib_modbus.h"

extern modbus_st uart0;
	
void drv_uart0_init(struct modbus* this);
uint8_t drv_uart0_read(master_slave_mode mode, struct modbus* this);
uint8_t drv_uart0_write(struct modbus* this);

#endif

