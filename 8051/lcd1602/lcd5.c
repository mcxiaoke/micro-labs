#include <reg51.h>

typedef unsigned char uint8;
typedef unsigned char bool;
typedef unsigned int uint16;

#define true 1
#define false 0

sbit LCD1602_RS=P0^0;
sbit LCD1602_RW=P0^1;
sbit LCD1602_E=P0^2;
#define LCD1602_PORT P2

static void lcd1602_delay(uint16 p_time)
{
    while(p_time--);
}

static void lcd1602_write_cmd(uint8 p_cmd)
{
    lcd1602_delay(100);
    LCD1602_E=0;
    lcd1602_delay(10);
    LCD1602_RS=0;
    lcd1602_delay(10);
    LCD1602_RW=0;
    lcd1602_delay(10);
    LCD1602_PORT=p_cmd;
    lcd1602_delay(10);
    LCD1602_E=1;
    lcd1602_delay(50);
    LCD1602_E=0;
}

static void lcd1602_write_data(uint8 p_data)
{
    lcd1602_delay(100);
    LCD1602_E=0;
    lcd1602_delay(10);
    LCD1602_RS=1;
    lcd1602_delay(10);
    LCD1602_RW=0;
    lcd1602_delay(10);
    LCD1602_PORT=p_data;
    lcd1602_delay(10);
    LCD1602_E=1;
    lcd1602_delay(50);
    LCD1602_E=0;
}

void lcd1602_init()
{
    uint8 t_cmd=0x38;
    lcd1602_write_cmd(t_cmd);
    t_cmd=0x06;
    lcd1602_write_cmd(t_cmd);
}

void lcd1602_display_switch(bool p_show_char,bool p_show_cursor,bool p_flash_cursor)
{
    uint8 t_cmd=0x08;
    if(p_show_char)
    {
        t_cmd|=0x04;
    }
    if(p_show_cursor)
    {
        t_cmd|=0x02;
    }
    if(p_flash_cursor)
    {
        t_cmd|=0x01;
    }
    lcd1602_write_cmd(t_cmd);
}

void lcd1602_cursor_shift(bool p_right)
{
    uint8 t_cmd;
    if(p_right)
    {
        t_cmd=0x14;
    }
    else
    {
        t_cmd=0x10;
    }
    lcd1602_write_cmd(t_cmd);
}

void lcd1602_clear()
{
    uint8 t_cmd=0x01;
    lcd1602_write_cmd(t_cmd);
}

void lcd1602_cursor_return()
{
    uint8 t_cmd=0x02;
    lcd1602_write_cmd(t_cmd);
}

void lcd1602_char_set(uint8 p_x,uint8 p_y,char p_char)
{
    uint8 t_cmd=0x80;
    p_x&=0x0f;
    p_y&=0x01;
    t_cmd|=p_x;
    if(p_y)
    {
        t_cmd|=0x40;
    }
    lcd1602_write_cmd(t_cmd);
    lcd1602_write_data(p_char);
}

void lcd1602_string_set(uint8 p_x,uint8 p_y,const char* p_string)
{
    uint8 t_x,t_y;
    p_x&=0x0f;
    p_y&=0x01;
    for(t_y=p_y;t_y<2;t_y++)
    {
        for(t_x=p_x;t_x<16&&(*p_string)!=0;t_x++)
        {
            lcd1602_char_set(t_x,t_y,*(p_string++));
        }
    }
}

void main()
{
    lcd1602_init();
    lcd1602_display_switch(true,true,true);
    lcd1602_string_set(0,0,"hello,");
    lcd1602_string_set(4,1,"world!");
    while(1);
}

// https://zhoujianshi.github.io/articles/2015/LCD1602%E7%9A%84%E4%BD%BF%E7%94%A8/index.html
