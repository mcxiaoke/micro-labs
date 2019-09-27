#include "i2c_lcd_hw_cfg.h"
#include "soft_i2c.h"
#include "I2C_LCD.h"

void main()
{
  KT_I2C_LCD_Init();
  KT_I2C_LCD_BackLight(1);
  KT_I2C_LCD_Clear();
  KT_I2C_LCD_Puts("Hello, World!");
  KT_I2C_LCD_NewLine();
  KT_I2C_LCD_Puts("LCD1602, PCF8574T");
  while(1);
}