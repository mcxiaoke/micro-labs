#include "lcd1602.h"

void delay_us(int i)
{
    while(i--);
}

void delay_ms(int i)
{
    int j;
    for(;i>0;i--)
        for(j=110;i>0;i--);
}

void lcd_cmd(unsigned char cmd)
{
    RS = 0;
    RW = 0;
    DB = cmd;
    delay_ms(1);
    EN = 1;
    delay_ms(1);
    EN = 0;
}

void lcd_dat(unsigned char dat)
{
    RS = 1;
    RW = 0;
    DB = dat;
    delay_ms(1);
    EN = 1;
    delay_ms(1);
    EN  = 0;
}

void lcd_init()
{
    RW = 0;
    EN = 0;
    lcd_cmd(0x38);
    lcd_cmd(0x0c);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
    lcd_cmd(0x41);
}