#ifndef _LIB_I2C_H_
#define _LIB_I2C_H_

#include <reg51.h>

sbit SCL = P0^6;
sbit SDA = P0^7;

void I2CInit();
void I2CStart();
void I2CRestart();
void I2CStop();
void I2CAck();
void I2CNak();
unsigned char I2CSend(unsigned char Data);
unsigned char I2CRead();

#endif
