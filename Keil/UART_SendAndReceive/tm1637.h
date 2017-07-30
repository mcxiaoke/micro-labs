#ifndef __TM1637_HEADER__
#define __TM1637_HEADER__

#include <reg51.h>
#include<intrins.h>

sbit CLK = P2^0;  //时钟信号
sbit DIO = P2^1;  //数据/地址数据

// for anode
extern unsigned char code ANODE[];

// for cathode
extern unsigned char code CATHODE[];
extern unsigned char code CATHODE_DOT[];

void TM1637_init( void );
void TM1637_start( void );
void TM1637_stop( void );
void TM1637_ask( void );
void TM1637_display_numbers(unsigned char* numbers, int length);
void TM1637_display_symbols(unsigned char* symbols, int length);
void TM1637_write1Bit(unsigned char mBit);
void TM1637_write1Byte(unsigned char mByte);
void TM1637_writeCommand(unsigned char mData);
void TM1637_writeData(unsigned char addr, unsigned char mData);

void delay_us(int us);
void delay_ms(int ms);

#endif