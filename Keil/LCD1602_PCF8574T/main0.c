//#include "lcd1602.h"
// from https://wenku.baidu.com/view/
// 12155a3edd3383c4ba4cd20d.html
#include <reg52.h>
#include <intrins.h>
sbit SCL = P2^0;
sbit SDA = P2^1;
bit ack;
unsigned char LCD_data;
unsigned char code digit[ ]={"0123456789"}; //定义字符数组显示数字
//*****************延时************************
void delay_nus(unsigned int n) //N us延时函数
{
	unsigned int i=0;
		for (i=0;i<n;i++)
			_nop_();
}
void delay_nms(unsigned int n) //N ms延时函数
{
	unsigned int i,j;
		for (i=0;i<n;i++)
			for (j=0;j<1140;j++);
}

void nop4()
{
	 _nop_();     //等待一个机器周期
	 _nop_();     //等待一个机器周期
	 _nop_();     //等待一个机器周期
	 _nop_();     //等待一个机器周期
}
//***************************************
void Start()
{
 	SDA=1;
    _nop_();
    SCL=1;
	nop4();
    SDA=0;
	nop4();
    SCL=0;
    _nop_();
	_nop_();
}
void Stop()
{
 	SDA=0;
    _nop_();	

	SCL=0;
	nop4();//>4us后SCL跳变
	SCL=1;
	nop4();
	SDA=1;
    _nop_();
    _nop_();
}
//******************************************
void  Write_A_Byte(unsigned char c)
{
 unsigned char BitCnt;
  for(BitCnt=0;BitCnt<8;BitCnt++)  //要传送的数据长度为8位
    {
     if((c<<BitCnt)&0x80)  SDA=1;   //判断发送位
     else  SDA=0;                
     _nop_();
     SCL=1;               //置时钟线为高，通知被控器开始接收数据位
     nop4(); 
     _nop_();       
     SCL=0; 
    }  
    _nop_();
    _nop_();
    SDA=1;               //8位发送完后释放数据线，准备接收应答位
    _nop_();
    _nop_();  
    SCL=1;
    _nop_();
    _nop_();
    _nop_();
    if(SDA==1)ack=0;     
       else ack=1;        //判断是否接收到应答信号
    SCL=0;
    _nop_();
    _nop_();
}

bit Write_Random_Address_Byte(unsigned char add,unsigned char dat)
{
 	Start();    //启动总线
	Write_A_Byte(add); //发送器件地址
    if(ack==0)return(0);
	Write_A_Byte(dat);   //发送数据
    if(ack==0)return(0);
	Stop(); //结束总线
    return(1);
}
//********************液晶屏使能*********************
void Enable_LCD_write()
{
    LCD_data|=(1<<(3-1));//E=1;
    Write_Random_Address_Byte(0x40,LCD_data);
    delay_nus(2);
    LCD_data&=~(1<<(3-1));//E=0;
    Write_Random_Address_Byte(0x40,LCD_data);
}

//*************写命令****************************
void LCD_write_command(unsigned char command)
{
	delay_nus(16);
	LCD_data&=~(1<<(1-1));//RS=0;
	LCD_data&=~(1<<(2-1));//RW=0;
    Write_Random_Address_Byte(0x40,LCD_data);

	LCD_data&=0X0f; //清高四位
	LCD_data|=command & 0xf0; //写高四位
    Write_Random_Address_Byte(0x40,LCD_data);
    Enable_LCD_write();

	command=command<<4; //低四位移到高四位
	LCD_data&=0x0f; //清高四位
	LCD_data|=command&0xf0; //写低四位
    Write_Random_Address_Byte(0x40,LCD_data);
    Enable_LCD_write();
}
//*************写数据****************************
void LCD_write_data(unsigned char value) 
{
	delay_nus(16);
	LCD_data|=(1<<(1-1));//RS=1;
	LCD_data&=~(1<<(2-1));//RW=0;
    Write_Random_Address_Byte(0x40,LCD_data);

	LCD_data&=0X0f; //清高四位
	LCD_data|=value&0xf0; //写高四位
    Write_Random_Address_Byte(0x40,LCD_data);
    Enable_LCD_write();

	value=value<<4; //低四位移到高四位
	LCD_data&=0x0f; //清高四位
	LCD_data|=value&0xf0; //写低四位
    Write_Random_Address_Byte(0x40,LCD_data);
    Enable_LCD_write();
}

//**********************设置显示位置*********************************
void set_position(unsigned char x,unsigned char y)
{
	unsigned char position;
	if (y == 0)
        position = 0x80 + x;
	else 
		position = 0xc0 + x;
    LCD_write_command(position);
}
//**********************显示字符串*****************************

void display_string(unsigned char x,unsigned char y,unsigned char *s)
{ 
    set_position(x,y);
	while (*s) 
 	{     
		LCD_write_data(*s);     
		s++;     
 	}
}
//*************液晶初始化****************************
void LCD_init(void) 
{ 
	LCD_write_command(0x28);
	delay_nus(40); 
	LCD_write_command(0x28);
	delay_nus(40); 
  //Enable_LCD_write();
	delay_nus(40);
	LCD_write_command(0x28); //4位显示！
	LCD_write_command(0x0c); //显示开
  delay_nms(2);
	LCD_write_command(0x01); //清屏
	delay_nms(2);
}
void main(void)
{
	LCD_init();
	display_string(4,0,"imxuheng"); //显示一段文字
	display_string(2,1,"Hello Today!"); //显示一段文字
	while(1);
}


void main0()
{
  //lcd_init();
  //lcd_display(4,0,"2017");
  //lcd_display(2,1, "Hello, World!");
  while(1);
}