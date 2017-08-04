#include <reg51.h>

#include "soft_i2c.h"

void I2C_Init(void) {
  SDA = 1;
  SCL = 1;
	Delay10us();
	Delay10us();
}

void I2C_Start(void) {
	SDA = 1;
	Delay10us();
	SCL = 1;
	Delay10us();
	SDA = 0;
	Delay10us();
	SCL = 0;
	Delay10us();
}

void I2C_Stop(void) {
	SCL = 0;
	Delay10us();
	SDA = 0;
	Delay10us();
	SCL = 1;
	Delay10us();
	SDA = 1;
	Delay10us();
}

uint8_t I2C_Write(uint8_t u8Data) {
	uint8_t i, ret;
	for(i=0; i<8; ++i) {
		if(u8Data&0x80) {
			SDA = 1;
		} else {
			SDA = 0;
		}
		Delay10us();
		SCL = 1;
		Delay10us();
		Delay10us();
		SCL = 0;
		Delay10us();
		u8Data<<=1;
	}

	SDA = 1;
	Delay10us();
	SCL = 1;
	Delay10us();
	if(SDA) {
		ret=1;
	} else {
		ret=0;
	}
	Delay10us();
	SCL = 0;
	Delay10us();
	return ret;
}

uint8_t I2C_Read(uint8_t u8Ack) {
	uint8_t i, ret;
	for(i=0; i<8; ++i) {
		ret<<=1;
		Delay10us();
		SCL = 1;
		Delay10us();
		if(SDA) {
			ret|=0x01;
		}
		Delay10us();
		SCL = 0;
		Delay10us();
	}

	if(u8Ack) {
		SDA = 1;
	} else {
		SDA = 0;
	}
	Delay10us();
	SCL = 1;
	Delay10us();
	Delay10us();
	SCL = 0;
	Delay10us();
	return ret;
}
