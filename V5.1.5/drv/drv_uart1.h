#ifndef DRV_UART1_H__
#define DRV_UART1_H__

#include "lib/lib_modbus.h"

extern modbus_st uart1;

void drv_uart1_init(struct modbus* this);
uint8_t drv_uart1_read(master_slave_mode mode, struct modbus* this);
uint8_t drv_uart1_write(struct modbus* this);

#endif

