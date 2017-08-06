#include "lcd1602.h"

static void delay_us(int i)
{
    while(i-->0);
}
static void delay_ms(int i)
{
    int k;
    for(;i>0;i--)
        for(k=110;k>0;k--);
}

static bit is_busy()
{
    DB = 0xFF;
    RS = 0;
    RW = 1;
    EN = 0;
    EN = 1;
    delay_us(10);
    return (bit) (DB & 0x80);//BF7
}

void lcd_write_command(unsigned char cmd)
{
    lcd_busy_wait();
    // H:Data, L:Command
    RS = 0; // select command register
    delay_us(10);
    // H:Read, L:Write
    RW = 0; // select write operation
    delay_us(10);
    // data strobe, active High
    DB = cmd; // send command to lcd
    EN = 1; // begin write circle
    delay_us(100); // pause for lcd reading
    EN = 0; // finish write circle
    delay_us(10);
}    

void lcd_write_data(unsigned char dat)
{
    lcd_busy_wait();
    // H:Data, L:Command
    RS = 1; // select data register
    delay_us(10);
    // H:Read, L:Write
    RW = 0; // select write operation
    delay_us(10);
    // data strobe, active High
    DB = dat; // send data to lcd
    EN = 1; // begin write circle
    delay_us(20); // pause for lcd reading
    EN = 0; // finish write circle
    delay_us(10);
}



void lcd_busy_wait()
{
    while(is_busy());
}

void lcd_init()
{
    // =0x38
    lcd_write_command(LCD_FUNCTIONSET|LCD_8BITMODE|LCD_2LINE|LCD_5x8DOTS);
    // 0x06
    lcd_write_command(LCD_ENTRYMODESET|LCD_ENTRYLEFT|LCD_ENTRYSHIFTDECREMENT);
    // = 0x0e
    lcd_write_command(LCD_DISPLAYCONTROL|LCD_DISPLAYON|LCD_CURSOROFF|LCD_BLINKOFF);
    //0x01
    lcd_write_command(LCD_CLEARDISPLAY);
}


// row:1,2 col:0-15
void lcd_set_cursor(unsigned char row, unsigned char col)
{
    if(row == 0){
        lcd_write_command(LCD_SETDDRAMADDR | col);
    }else{
        lcd_write_command(LCD_SETDDRAMADDR | (0x40+col));
    }
}

void lcd_display_string(unsigned char row, unsigned char col, unsigned char *str)
{
    lcd_set_cursor(row, col);
    while(*str){
        lcd_write_data(*str++);
    }
}

void lcd_clear_display()
{
    lcd_write_command(LCD_CLEARDISPLAY);
    delay_us(20);
}

void lcd_cursor_home()
{
    lcd_write_command(LCD_RETURNHOME);
}

void lcd_move_right(unsigned char count)
{
    while(count-->0){
        lcd_write_command(LCD_CURSORSHIFT|LCD_DISPLAYMOVE|LCD_MOVERIGHT);
        delay_ms(50);
    }
}

void lcd_move_left(unsigned char count)
{
    while(count-->0){
        lcd_write_command(LCD_CURSORSHIFT|LCD_DISPLAYMOVE|LCD_MOVELEFT);
        delay_ms(50);
    }
}