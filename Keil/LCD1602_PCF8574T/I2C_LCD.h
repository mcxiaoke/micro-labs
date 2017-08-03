#ifndef _KT_I2C_LCD_
#define _KT_I2C_LCD_

#include "i2c_lcd_hw_cfg.h"
#include "soft_i2c.h"

static uint8_t u8LCD_Buff[8];//bo nho dem luu lai toan bo
static uint8_t u8LcdTmp;

#define	MODE_4_BIT		0x28
#define	CLR_SCR			0x01
#define	DISP_ON			0x0C
#define	CURSOR_ON		0x0E
#define	CURSOR_HOME		0x80

void LCD_Write_4bit(uint8_t u8Data);
void FlushVal(void);
void KT_I2C_LCD_WriteCmd(uint8_t u8Cmd);

	void KT_I2C_LCD_Init(void);
	void KT_I2C_LCD_WriteCmd(uint8_t u8Cmd);
	void KT_I2C_LCD_Puts(char *szStr);
	void KT_I2C_LCD_Clear(void);
	void KT_I2C_LCD_NewLine(void);
	void KT_I2C_LCD_BackLight(uint8_t u8BackLight);
#endif
