#ifndef __AT24C_HEADER__
#define __AT24C_HEADER__

#include <reg51.h>
#include <intrins.h>

sbit SDA = P2^1;
sbit SCL = P2^0;

void I2CWait();
void I2CDelay();
void I2CInit();
void I2CStart();
void I2CRestart();
void I2CStop();
void I2CAck();
void I2CNak();
unsigned char I2CSend(unsigned char Data);
unsigned char I2CRead();


#endif