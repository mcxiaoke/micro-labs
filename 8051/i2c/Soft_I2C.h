#ifndef _SOFT_I2C_
#define _SOFT_I2C_

#include <reg51.h>

typedef unsigned char uint8_t;
typedef unsigned long uint32_t;

sbit SDA = P1^2;
sbit SCL = P1^1;
sbit WP = P1^0;
  
	void I2C_Start(void);
	void I2C_Stop(void);
	uint8_t I2C_Write(uint8_t u8Data);
	uint8_t I2C_Read(uint8_t u8Ack);
	void I2C_Init(void);
#endif
