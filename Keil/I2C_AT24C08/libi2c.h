#ifndef _libi2ch_
#define _libi2ch_
#include<reg51.h>
#define uchar unsigned char
#define uint unsigned int
#define write_ADD 0xa0
#define read_ADD 0xa1
uchar a;  
sbit SDA=P2^1;
sbit SCL=P2^0;
void SomeNop();
void init();
void check_ACK(void)；
void I2CStart(void);
void I2cStop(void);
void write_byte(uchar dat);
void delay(uint z);
uchar read_byte();
void write(uchar addr,uchar dat);
uchar read(uchar addr);

#endif