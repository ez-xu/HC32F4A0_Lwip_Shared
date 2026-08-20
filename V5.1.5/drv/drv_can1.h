#ifndef DRV_CAN1_H__
#define DRV_CAN1_H__

#include "lib/lib_can.h"

extern can_handle_st can1;

void drv_can1_init(can_handle_st *this, uint32_t baudrate);
uint8_t drv_can1_receive(can_handle_st *this, uint32_t *id, uint8_t *data);
uint8_t drv_can1_receive_detail(can_handle_st *this, frame_st *frame, uint16_t ms);
uint8_t drv_can1_send(can_handle_st *this, uint32_t id, uint8_t *data);
uint8_t drv_can1_send_detail(can_handle_st *this, frame_st *frame, uint16_t ms);    

uint8_t can_filter(uint32_t id, uint8_t *data);

#endif


