#include<reg51.h>
#include "lcd1602.h"

/* Configure the data bus and Control pins as per the hardware connection
   Databus is connected to P2_0:P2_7 and control bus P0_0:P0_2*/
#define LcdDataBus  P2
sbit LCD_RS = P3^0;
sbit LCD_RW = P3^1;
sbit LCD_EN = P3^2;

/* local function to generate delay */
static void delay_us(int cnt)
{
    int i;
    for(i=0;i<cnt;i++);
}

static void delay_ms(int i)
{
    int k;
    for(;i>0;i--)
        for(k=110;k>0;k--);
}

/* Function to send the command to LCD */
void Lcd_CmdWrite( char cmd)
{
    LcdDataBus=cmd;    // Send the command to LCD
    LCD_RS=0;          // Select the Command Register by pulling RS LOW
    LCD_RW=0;          // Select the Write Operation  by pulling RW LOW
    LCD_EN=1;          // Send a High-to-Low Pusle at Enable Pin
    delay_us(10);
    LCD_EN=0;
    delay_us(1000);
}

/* Function to send the Data to LCD */
void Lcd_DataWrite( char dat)
{
    LcdDataBus=dat;   // Send the data to LCD
    LCD_RS=1;         // Select the Data Register by pulling RS HIGH
    LCD_RW=0;          // Select the Write Operation by pulling RW LOW
    LCD_EN=1;         // Send a High-to-Low Pusle at Enable Pin
    delay_us(10);
    LCD_EN=0;
    delay_us(1000);
}

void main()
{
    unsigned char i;
    lcd_init();
    // all: 00h-27h displayed:00h-0fh
    lcd_display_string(0,20,"Hello 1602!");
    lcd_display_string(1,20,"2017-06-30 16:30");
    lcd_move_right(60);
    while(1);
}

void main0()
{
    char i,a[]={"Good morning!"};

    Lcd_CmdWrite(0x38);        // enable 5x7 mode for chars
    Lcd_CmdWrite(0x0E);        // Display OFF, Cursor ON
    Lcd_CmdWrite(0x01);        // Clear Display
    Lcd_CmdWrite(0x80);        // Move the cursor to beginning of first line

    Lcd_DataWrite('H');
    Lcd_DataWrite('e');
    Lcd_DataWrite('l');
    Lcd_DataWrite('l');
    Lcd_DataWrite('o');
    Lcd_DataWrite(' ');
    Lcd_DataWrite('w');
    Lcd_DataWrite('o');
    Lcd_DataWrite('r');
    Lcd_DataWrite('l');
    Lcd_DataWrite('d');

    Lcd_CmdWrite(0xc0);        //Go to Next line and display Good Morning
    for(i=0;a[i]!=0;i++)
    {
        Lcd_DataWrite(a[i]);
    }

    while(1);
}
