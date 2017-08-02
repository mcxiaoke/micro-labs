#ifndef _KT_I2C_LCD_CFG_
	#define _KT_I2C_LCD_CFG_
  
#include <reg51.h>
  sbit SDA = P2^1;
  sbit SCL = P2^0;

	void Delay10us(void);
  // addr = 0x27 << 1 = 0x4e
	#define I2C_LCD_ADDR 0x4E
	#define LCD_EN 2
	#define LCD_RW 1
	#define LCD_RS 0
	#define LCD_D4 4
	#define LCD_D5 5
	#define LCD_D6 6
	#define LCD_D7 7
	#define LCD_BL 3
#endif
