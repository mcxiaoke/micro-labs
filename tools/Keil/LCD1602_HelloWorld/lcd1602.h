#ifndef __LCD1602_H__
#define __LCD1602_H__

#include <reg51.h>
#include <intrins.h>
#include <stdio.h>
#include <string.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

// flags for address of two rows
#define LCD_ADDRESS_ROW_1 0x80
#define LCD_ADDRESS_ROW_2 0xc0

#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit

#define LCD_1602 0x38

#define DB P2
sbit RS = P3^0;
sbit RW = P3^1;
sbit EN = P3^2;

void lcd_init();
void lcd_busy_wait();
void lcd_write_command(unsigned char cmd);
void lcd_write_data(unsigned char dat);
void lcd_display_string(unsigned char row, unsigned char col, unsigned char *str);
void lcd_set_cursor(unsigned char row, unsigned char col);
void lcd_clear_display();
void lcd_cursor_home();
void lcd_move_left(unsigned char count);
void lcd_move_right(unsigned char count);

#endif