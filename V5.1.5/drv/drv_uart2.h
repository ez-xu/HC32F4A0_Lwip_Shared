#ifndef DRV_UART2_H__
#define DRV_UART2_H__

#include "lib/lib_modbus.h"

extern modbus_st uart2;

void drv_uart2_init(struct modbus* this);
uint8_t drv_uart2_read(master_slave_mode mode, struct modbus* this);
uint8_t drv_uart2_write(struct modbus* this);

#endif

