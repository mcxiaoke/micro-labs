
#ifndef __LCD1602_HEADER__
#define __LCD1602_HEADER__

#include <reg51.h>
#include <intrins.h>

sbit SCL = P2^0;
sbit SDA = P2^1;
  
//extern unsigned char lcd_data;  
extern unsigned char code digit[];

void nop4();
void delay_us(int i);
void delay_ms(int i);

void i2c_start();
void i2c_stop();
void i2c_write(unsigned char c);
bit i2c_write_addr(unsigned char addr, unsigned char dat);
void lcd_enable_write();
void lcd_write_cmd(unsigned char cmd);
void lcd_write_data(unsigned char value);
void lcd_set_cursor(unsigned char x, unsigned char y);
void lcd_init();
void lcd_display(unsigned char x, 
  unsigned char y, 
    unsigned char* s);

#endif

