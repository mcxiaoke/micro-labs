#ifndef __I2C_DS1307_HEADER__
#define __I2C_DS1307_HEADER__

#include <reg51.h>
#include <intrins.h>
#include  <stdio.h>

sbit SCL = P2^0;
sbit SDA = P2^1;

void delay_us(int i);
void delay_ms(int i);
void i2c_init();
void i2c_start();
void i2c_stop();
void i2c_ack(bit ack);
bit i2c_write(unsigned char dat);
unsigned char i2c_read_nack();
unsigned char i2c_read_ack();

#endif