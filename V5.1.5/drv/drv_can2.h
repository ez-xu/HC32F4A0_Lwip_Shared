
#ifndef DRV_CAN2_H__
#define DRV_CAN2_H__

#include "lib/lib_can.h"

extern can_handle_st can2;

void drv_can2_init(can_handle_st *this, uint32_t baudrate);
uint8_t drv_can2_receive(can_handle_st *this, uint32_t *id, uint8_t *data);
uint8_t drv_can2_receive_detail(can_handle_st *this, frame_st *frame, uint16_t ms);
uint8_t drv_can2_send(can_handle_st *this, uint32_t id, uint8_t *data);
uint8_t drv_can2_send_detail(can_handle_st *this, frame_st *frame, uint16_t ms);    

#endif


