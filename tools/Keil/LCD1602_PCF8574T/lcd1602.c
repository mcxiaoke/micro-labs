#include "lcd1602.h"

bit ack;
unsigned char dat;
unsigned char code digit[]={"0123456789"};

void nop4()
{
  _nop_();
  _nop_();
  _nop_();
  _nop_();
}

void delay_us(int i)//1ns
{
    for(; i>0; i--)
      _nop_();
}

void delay_ms(int i)//1ms
{
    unsigned char x,j;
    for(j=0;j<i;j++)
    for(x=0;x<=110;x++);
}

void i2c_start()
{
  SDA = 1;
  _nop_();
  SCL = 1;
  nop4();
  SDA = 0;
  nop4();
  SCL = 0;
  _nop_();
  _nop_();
}

void i2c_stop()
{
  SDA = 0;
  _nop_();
  SCL = 0;
  nop4();
  SCL = 1;
  nop4();
  SDA = 1;
  _nop_();
  _nop_();
}

void i2c_write(unsigned char c)
{
  unsigned char i;
  for(i=0;i<8;i++){
    if((c<<i)&0x80){
      SDA = 1;
    }else{
      SDA = 0;
    }
    _nop_();
    SCL = 1;
    nop4();
    _nop_();
    SCL = 0;
  }
  _nop_();
  _nop_();
  SDA = 1;
  _nop_();
  _nop_();
  SCL = 1;
  _nop_();
  _nop_();
  _nop_();
  if(SDA == 1){
   ack = 0;
  }else{
    ack = 1;
  }
  SCL = 0;
  _nop_();
  _nop_();
}

bit i2c_write_addr(unsigned char addr, unsigned char dat)
{
  i2c_start();
  i2c_write(addr);
  if(ack==0){
    return 0;
  }
  i2c_write(dat);
  if(ack==0){
    return 0;
  }
  i2c_stop();
  return 1;  
}

void lcd_enable_write()
{
  //unsigned char dat;
  dat |= (1<<(3-1));//E=1
  i2c_write_addr(0x40, dat);
  delay_us(2);
  dat &= ~(1<<(3-1));//E=0;
  i2c_write_addr(0x40, dat);
}

void lcd_write_cmd(unsigned char cmd)
{
  //unsigned char dat;
  delay_us(16);
  dat &= ~(1<<(1-1));//RS=0;
  dat &= ~(1<<(2-1));//RW=0;
  i2c_write_addr(0x40, dat);
  
  dat &= 0X0f; //clear high 4bit  
  dat |= cmd&0xf0; // write high 4bit 
  i2c_write_addr(0x40,dat); 
  lcd_enable_write();
  
  cmd=cmd<<4; //low -> high 4bit  
  dat &= 0x0f; // clear high 4bit  
  dat |= cmd&0xf0; // write low 4bit 
  i2c_write_addr(0x40,dat); 
  lcd_enable_write(); 
}

void lcd_write_data(unsigned char value)
{
  //unsigned char dat;
  delay_us(16);
  dat|=(1<<(1-1));//RS=1;  
  dat&=~(1<<(2-1));//RW=0;  
  i2c_write_addr(0x40,dat);    
  dat&=0X0f; //clear high 4bit  
  dat|=value&0xf0; // write high 4bit  
  i2c_write_addr(0x40,dat); 
  lcd_enable_write();    
  
  value=value<<4; //low->high 4bit  
  dat&=0x0f; // clear high 4bit  
  dat|=value&0xf0; //write low 4bit  
  i2c_write_addr(0x40,dat); 
  lcd_enable_write();
}

void lcd_set_cursor(unsigned char x, unsigned char y)
{
  unsigned char pos;
  if(y==0){
    pos = 0x80 + x;
  }else{
    pos = 0xc0 + x;
  }
  lcd_write_cmd(pos);
}

void lcd_init()
{
  lcd_write_cmd(0x28);
  delay_us(40);
  lcd_write_cmd(0x28);
  delay_us(40);
  lcd_enable_write();
  delay_us(40);
  lcd_write_cmd(0x28);
  lcd_write_cmd(0x0c);
  lcd_write_cmd(0x01);
  delay_ms(2);
}

void lcd_display(unsigned char x, 
  unsigned char y, 
    unsigned char* s)
{
  lcd_set_cursor(x, y);
  while(*s){
    lcd_write_data(*s);
    s++;
  }
}     
















