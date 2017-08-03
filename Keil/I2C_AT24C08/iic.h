#ifndef __iic_header__
#define __iic_header__
// https://code.csdn.net/snippets/1888153/
// http://blog.csdn.net/fighting_kangle/article/details/52644648
// http://blog.csdn.net/lxl123/article/details/22884719
#include <reg51.h>
#include <intrins.h>

#define SUCC 1
#define ERR 0

sbit SDA = P2^1;
sbit SCL = P2^0;

void delay_us(int i);
void iic_wait();
void iic_start();
void iic_stop();
bit iic_send_byte(unsigned char byte);
unsigned char iic_receive_byte();
void iic_ack();
void iic_noack();
bit iic_send_str(unsigned char addr, unsigned char pos, unsigned char dat, bit stop);
bit iic_receive_str(unsigned char addr, unsigned char pos, unsigned char* str, unsigned char len);

#endif