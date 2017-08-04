#ifndef __iic_header__
#define __iic_header__
// https://code.csdn.net/snippets/1888153/
// http://blog.csdn.net/fighting_kangle/article/details/52644648
// http://blog.csdn.net/lxl123/article/details/22884719
#include <reg51.h>
#include <intrins.h>

#define SUCC 1
#define ERR 0

sbit SCL = P2^0;
sbit SDA = P2^1;

void i2c_delay();
void i2c_init();
void i2c_wait();
void i2c_start();
void i2c_stop();
bit i2c_write(unsigned char byte);
unsigned char iic_read();
void i2c_ack();
void i2c_noack();
bit i2c_write_str(unsigned char addr, unsigned char pos, unsigned char *str, unsigned char len);
bit i2c_read_str(unsigned char addr, unsigned char pos, unsigned char* str, unsigned char len);

#endif