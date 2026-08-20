#ifndef _MP2797_I2C_H
#define _MP2797_I2C_H
#include "main.h"

uint8_t CRC8(uint8_t* data, uint16_t length);

void I2C_Initialize(void);
void I2C_Write16B(uint8_t Register, uint8_t DataL, uint8_t DataH, uint8_t usecrc, uint8_t add_delay);
uint8_t I2C_Read16B(uint8_t Register, uint8_t* DataL, uint8_t* DataH, uint8_t usecrc, uint8_t add_delay);

#endif
