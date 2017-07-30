 /*******************************
 **** Describe: TM1673控制芯片，可以设置时间的倒计时(定时不准，可以自己校准)
 ****  四个数码管0xc0,0xc1,0xc2,0xc3
 ****     Time: 2015.05.14
 ****   Author: zys
 ********************************/
#include "tm1637.h"

// for anode
unsigned char code ANODE[] = {
	0xc0,0xf9,0xa4,0xb0,
	0x99,0x92,0x82,0xf8,
	0x80,0x90,0x88,0x83,
	0xc6,0xa1,0x86,0x8e
};

// for cathode
unsigned char code CATHODE[] = {
    // 0011 1111, 0000 0110
	0x3f,0x06,0x5b,0x4f,
	0x66,0x6d,0x7d,0x07,
	0x7f,0x6f,0x77,0x7c,
	0x39,0x5e,0x79,0x71
};

// dot for cathode, only 0xc1
unsigned char code CATHODE_DOT[]={0xbf,0x86,0xdb,0xcf,0xe6,0xed,0xfd,0x87,0xff,0xef}; //有小数点只用于地址0xc1

/**
 init setup
**/
void TM1637_init( void )
{
    TM1637_start();
    // light level 0x88-0x8f
    TM1637_writeCommand(0x88);
     TM1637_stop();
}


/********************************************************************
* 名称 : void TM1637_start( void )
* 功能 : start信号
* 输入 : void
* 输出 : 无
**************************************************************/
void TM1637_start( void )
{
    CLK = 1;
    DIO = 1;
    delay_us(4);
    DIO = 0;
    delay_us(4);
    //CLK = 0;
    //delay_us(4);
}

/********************************************************************
* 名称 : void TM1637_stop( void )
* 功能 : stop信号
* 输入 : void
* 输出 : 无
**************************************************************/
void TM1637_stop( void )
{
    CLK = 0;
    delay_us(4);
    DIO = 0;
    delay_us(4);
    CLK = 1;
    delay_us(4);
    DIO = 1;
    //delay_us(4);
}

/**
 i2c response
**/
void TM1637_ask( void )
{
    CLK = 0;
    delay_us(4);
    while(DIO);
    CLK = 1;
    delay_us(4);
    CLK = 0;
}

/********************************************************************
* 名称 : void TM1637_write1Bit(unsigned char mBit )
* 功能 : 写1bit
* 输入 : unsigned char mBit
* 输出 : 无
**************************************************************/
void TM1637_write1Bit(unsigned char mBit )
{
    CLK = 0;
    //delay_us(4);
    if(mBit)
        DIO = 1;
    else
        DIO = 0;
    delay_us(4);
    CLK = 1;
    delay_us(4);
}

/********************************************************************
* 名称 : void TM1637_write1Byte(unsigned char mByte)
* 功能 : 写1byte
* 输入 : unsigned char mByte
* 输出 : 无
**************************************************************/
void TM1637_write1Byte(unsigned char mByte)
{
    char loop = 0;
    for(loop = 0; loop < 8; loop++)
    {
        TM1637_write1Bit((mByte>>loop)&0x01); //取得最低位
    }
    TM1637_ask();
}

/********************************************************************
* 名称 : void TM1637_writeCammand(unsigned char mData)
* 功能 : 写指令1byte
* 输入 : unsigned char mData
* 输出 : 无
**************************************************************/
void TM1637_writeCommand(unsigned char mData)
{
    TM1637_start();
    TM1637_write1Byte(mData);  //数据
    TM1637_stop();
}

/********************************************************************
* 名称 : void TM1637_writeData(unsigned char addr, unsigned char mData)
* 功能 : 固定地址写数据1byte
* 输入 : unsigned char addr, unsigned char mData
* 输出 : 无
**************************************************************/
void TM1637_writeData(unsigned char addr, unsigned char mData)
{
    TM1637_start();
    TM1637_write1Byte(addr);  //地址
    TM1637_write1Byte(mData);  //数据
    TM1637_stop();
}

/********************************************************************
* 名称 : Delay_1ms(unsigned int i)
* 功能 : 延时子程序，延时时间为 us
* 输入 :
* 输出 : 无
**************************************************************/
void delay_us(int i)
{
    for(; i>0; i--)
      _nop_();
}

/********************************************************************
* 名称 : Delay_1ms(unsigned int i)
* 功能 : 延时子程序，延时时间为 1ms * x
* 输入 : x (延时一毫秒的个数)
* 输出 : 无
***********************************************************************/
void delay_ms(int i)//1ms延时
{
    unsigned char x,j;
    for(j=0;j<i;j++)
    for(x=0;x<=110;x++);
}
