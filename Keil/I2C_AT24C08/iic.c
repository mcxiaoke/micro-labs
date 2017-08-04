/*==========================================================================
  名称：IIC协议 
  编写：aiken
  修改：无
  内容：函数是采用软件延时的方法产生SCL脉冲，固对高晶振频率要作一定的修改
       (本例是1us机器周期，即晶振频率要小于12MHZ)
============================================================================*/ 

#include "iic.h"

bit ack;

void iic_wait()
{
  unsigned char i;
  while((SCL==0)&&(++i<250));
}

void iic_delay()
{
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}

void iic_init()
{
    SDA = 1;
    SCL = 1;
    iic_delay();
}

/*=================================================
                    启动总线
==================================================*/
void iic_start()
{
    SDA = 1;       //发送起始条件的数据信号
    iic_delay();
    SCL = 1;
    iic_delay();   //起始条件建立时间大于4.7us,延时
    SDA = 0;       //发送起始信号
    iic_delay();   //起始条件锁定时间大于4μ
    SCL = 0;       //钳住I2C总线，准备发送或接收数据
}

/*=================================================
                    结束总线
==================================================*/
void iic_stop()
{
    SDA = 0;       //发送结束条件的数据信号
    iic_delay();
    SCL = 1;
    iic_delay();   //结束条件建立时间大于4μ
    SDA = 1;       //发送I2C总线结束信号
    iic_delay();
}

/*=====================================================================
                       发送一个字节数据               
函数原型: bit iic_send_byte(unsigned char byte);
功能: 将数据byte发送出去,可以是地址,也可以是数据,发完后等待应答,并对
      此状态位进行操作.(不应答或非应答都使ack=0 假)     
      发送数据正常，ack=1; ack=0表示被控器无应答或损坏。
======================================================================*/
bit iic_send_byte(unsigned char byte)
{
    unsigned char i;
	
    for(i = 0; i < 8; i++)    //要传送的数据长度为8位
    {
        SDA = byte & 0x80;    //判断发送位
        SCL = 1;              //置时钟线为高，通知被控器开始接收数据位
        iic_delay();          //保证时钟高电平周期大于4μ
        SCL = 0;
        byte <<= 1;
    }
	
	SCL = 1;
	SDA = 1;                  //8位发送完后释放数据线，准备接收应答位
    //iic_wait();
	iic_delay();
	if(0 == SDA)              //判断是否接收到应答信号
	{
	    ack = 1;
	}
	else
	{
	    ack = 0;
	}
	
	SCL = 0;
	return ack;
}

/*===================================================================================
                             接受一个字节数据               
函数原型: unsigned char iic_receive_byte();
功能:  用来接收从器件传来的数据,并判断总线错误(不发应答信号)，发完后请用应答函数。  
====================================================================================*/	
unsigned char iic_receive_byte()
{
    unsigned char i;
	unsigned char a;
	unsigned char temp = 0;
	
	SDA = 1;                      //置数据线为输入方式
	
	for(i = 0; i < 8; i++)
	{
	    SCL = 0;                  //置时钟线为低，准备接收数据位
		iic_delay();              //时钟低电平周期大于4.7us
		SCL = 1;                  //置时钟线为高使数据线上数据有效
		
		if(SDA)
		{
		    a = 1;
		}
		else
		{ 
		    a = 0;
		}
		
		temp |= (a << (7 - i));   //读数据位,接收的数据位放入retc中
		iic_delay();
	}
	SCL = 0;
	return temp;
}

/*===============================================================
                         应答子函数
================================================================*/
void iic_ack()
{
    SDA = 0;
	SCL = 1;
	iic_delay();    //时钟低电平周期大于4μ
	SCL = 0;        //清时钟线，钳住I2C总线以便继续接收
}

/*================================================================
                        非应答子函数
=================================================================*/
void iic_noack()
{
    SDA = 1;
	SCL = 1;
	iic_delay();    //时钟低电平周期大于4μ
	SCL = 0;        //清时钟线，钳住I2C总线以便继续接收
}

/*===========================================================================================
                       向有子地址器件发送多字节数据函数               
函数原型: bit iic_send_str(unsigned char addr, unsigned char pos, unsigned char *str, unsigned char len);  
功能: 从启动总线到发送地址，子地址，数据，结束总线的全过程。
      从器件地址addr，子地址pos，发送内容是str指向的内容，发送len个字节。
      如果返回1表示操作成功，否则操作有误。
注意：使用前必须已结束总线。
=============================================================================================*/
bit iic_send_str(unsigned char addr, unsigned char pos, unsigned char *str, unsigned char len)
{
    unsigned char i;
	
	iic_start();                //启动总线
	
	iic_send_byte(addr);         //发送器件地址
	if(0 == ack)
	{
	    return ERR;
	}
	
	iic_send_byte(pos);        //发送器件子地址
	if(0 == ack)
	{
	    return ERR;
	}
	
	for(i = 0; i < len; i++)
	{
	    iic_send_byte(*str);    //发送数据
		iic_delay();            //必须延时等待芯片内部自动处理数据完毕
		
		if(0 == ack)
		{
		    return ERR;
		}
		
		str++;
	}
	
	iic_stop();                 //结束总线
	return SUCC;
}

/*===========================================================================================
                        向有子地址器件读取多字节数据函数               
函数原型: bit iic_receive_str(unsigned char addr, unsigned char pos, unsigned char *str, unsigned char len); 
功能: 从启动总线到发送地址，子地址，读数据，结束总线的全过程。
      从器件地址addr，子地址pos，读出的内容放入str指向的存储区，读len个字节。
      如果返回1表示操作成功，否则操作有误。
注意：使用前必须已结束总线。
=============================================================================================*/
bit iic_receive_str(unsigned char addr, unsigned char pos, unsigned char* str, unsigned char len)
{
    unsigned char i;
	
	iic_start();                     //启动总线
	
	iic_send_byte(addr);              //发送器件地址
	if(0 == ack)
	{
	    return ERR;
	}
	
	iic_send_byte(pos);             //发送器件子地址
	if(0 == ack)
	{
	    return ERR;
	}
	
	iic_start();
	iic_send_byte(addr + 1);
	if(0 == ack)
	{
	    return ERR;
	}
	
	for(i = 0; i < len - 1; i++)
	{
	    *str = iic_receive_byte();   //发送数据
		iic_ack();                   //发送就答位 
		str++;
	}
	
	*str = iic_receive_byte();
	iic_noack();                     //发送非应位
	iic_stop();                      //结束总线
	return SUCC;
}