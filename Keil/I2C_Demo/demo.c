#include<reg51.h>
#include<intrins.h>
#include<stdio.h>
#include<string.h>
#include "debug.h"

// modifed http://blog.csdn.net/wqx521/article/details/50801547
#define DS1307_CTRL_ID 0x68
#define DS1307_EEPROM	0xD0
#define uint8_t unsigned char
#define I2CDelay()  {_nop_();_nop_();_nop_();_nop_();}
sbit I2C_SCL = P0 ^ 6;
sbit I2C_SDA = P0 ^ 7;

// Convert Decimal to Binary Coded Decimal (BCD)
uint8_t dec2bcd(uint8_t num)
{
  return ((num/10 * 16) + (num % 10));
}

// Convert Binary Coded Decimal (BCD) to Decimal
uint8_t bcd2dec(uint8_t num)
{
  return ((num/16 * 10) + (num % 16));
}

void I2CInit()
{
  I2C_SDA = 1; //确保SDA、SCL都是高电平
  I2C_SCL = 1;
}

/* 产生总线起始信号 */
void I2CStart()
{
  I2C_SDA = 1; //首先确保SDA、SCL都是高电平
  I2C_SCL = 1;
  I2CDelay();
  I2C_SDA = 0; //先拉低SDA
  I2CDelay();
  I2C_SCL = 0; //再拉低SCL
}
/* 产生总线停止信号 */
void I2CStop()
{
  I2C_SCL = 0; //首先确保SDA、SCL都是低电平
  I2C_SDA = 0;
  I2CDelay();
  I2C_SCL = 1; //先拉高SCL
  I2CDelay();
  I2C_SDA = 1; //再拉高SDA
  I2CDelay();
}
/* I2C总线写操作，dat-待写入字节，返回值-从机应答位的值 */
bit I2CWrite(unsigned char dat)
{
  bit ack; //用于暂存应答位的值
  unsigned char mask;  //用于探测字节内某一位值的掩码变量

  for (mask = 0x80; mask != 0; mask >>= 1) //从高位到低位依次进行
  {
    if ((mask & dat) == 0) //该位的值输出到SDA上
    {
      I2C_SDA = 0;
    }
    else
    {
      I2C_SDA = 1;
    }
    I2CDelay();
    I2C_SCL = 1;          //拉高SCL
    I2CDelay();
    I2C_SCL = 0;          //再拉低SCL，完成一个位周期
  }
  I2C_SDA = 1;   //8位数据发送完后，主机释放SDA，以检测从机应答
  I2CDelay();
  I2C_SCL = 1;   //拉高SCL
  ack = I2C_SDA; //读取此时的SDA值，即为从机的应答值
  I2CDelay();
  I2C_SCL = 0;   //再拉低SCL完成应答位，并保持住总线

  return (~ack); //应答值取反以符合通常的逻辑：
  //0=不存在或忙或写入失败，1=存在且空闲或写入成功
}
/* I2C总线读操作，并发送非应答信号，返回值-读到的字节 */
unsigned char I2CReadNAK()
{
  unsigned char mask;
  unsigned char dat;

  I2C_SDA = 1;  //首先确保主机释放SDA
  for (mask = 0x80; mask != 0; mask >>= 1) //从高位到低位依次进行
  {
    I2CDelay();
    I2C_SCL = 1;      //拉高SCL
    if(I2C_SDA == 0)  //读取SDA的值
    {
      dat &= ~mask;  //为0时，dat中对应位清零
    }
    else
    {
      dat |= mask;  //为1时，dat中对应位置1
    }
    I2CDelay();
    I2C_SCL = 0;      //再拉低SCL，以使从机发送出下一位
  }
  I2C_SDA = 1;   //8位数据发送完后，拉高SDA，发送非应答信号
  I2CDelay();
  I2C_SCL = 1;   //拉高SCL
  I2CDelay();
  I2C_SCL = 0;   //再拉低SCL完成非应答位，并保持住总线

  return dat;
}
/* I2C总线读操作，并发送应答信号，返回值-读到的字节 */
unsigned char I2CReadACK()
{
  unsigned char mask;
  unsigned char dat;

  I2C_SDA = 1;  //首先确保主机释放SDA
  for (mask = 0x80; mask != 0; mask >>= 1) //从高位到低位依次进行
  {
    I2CDelay();
    I2C_SCL = 1;      //拉高SCL
    if(I2C_SDA == 0)  //读取SDA的值
    {
      dat &= ~mask;  //为0时，dat中对应位清零
    }
    else
    {
      dat |= mask;  //为1时，dat中对应位置1
    }
    I2CDelay();
    I2C_SCL = 0;      //再拉低SCL，以使从机发送出下一位
  }
  I2C_SDA = 0;   //8位数据发送完后，拉低SDA，发送应答信号
  I2CDelay();
  I2C_SCL = 1;   //拉高SCL
  I2CDelay();
  I2C_SCL = 0;   //再拉低SCL完成应答位，并保持住总线

  return dat;
}

/* 读取函数，buf-数据接收指针，addr-E2中的起始地址，len-读取长度 */
void DS1307Read(unsigned char *buf, unsigned char addr, unsigned char len)
{
  do                         //用寻址操作查询当前是否可进行读写操作
  {
    I2CStart();
    if(I2CWrite(DS1307_EEPROM << 1)) //应答则跳出循环，非应答则进行下一次查询
    {
      break;
    }
    I2CStop();
  }
  while(1);
  I2CWrite(addr);            //写入起始地址
  I2CStart();                //发送重复启动信号
  I2CWrite((DS1307_EEPROM << 1) | 0x01); //寻址器件，后续为读操作
  while (len > 1)           //连续读取len-1个字节
  {
    *buf++ = I2CReadACK(); //最后字节之前为读取操作+应答
    len--;
  }
  *buf = I2CReadNAK();      //最后一个字节为读取操作+非应答
  I2CStop();
}
/* 写入函数，buf-源数据指针，addr-E2中的起始地址，len-写入长度 */
void DS1307Write(unsigned char *buf, unsigned char addr, unsigned char len)
{
  while (len > 0)
  {
    //等待上次写入操作完成
    do                        //用寻址操作查询当前是否可进行读写操作
    {
      I2CStart();
      if(I2CWrite(DS1307_EEPROM << 1)) //应答则跳出循环，非应答则进行下一次查询
      {
        break;
      }
      I2CStop();
    }
    while(1);
    //按页写模式连续写入字节
    I2CWrite(addr);           //写入起始地址
    while(len > 0)
    {
      I2CWrite(*buf++);     //写入一个字节数据
      len--;                //待写入长度计数递减
      addr++;               //E2地址递增
      if ((addr & 0x07) == 0) //检查地址是否到达页边界，24C02每页8字节，
      {
        //所以检测低3位是否为零即可
        break;            //到达页边界时，跳出循环，结束本次写操作
      }
    }
    I2CStop();
  }
}

void delay_1s()
{
    unsigned int i, j, k;
    for (i = 88; i > 0; i--)
        for (j = 71; j > 0; j--)
            for (k = 40; k > 0; k--)
                ;
}

void main()
{  
    uint8_t str[32];
    uint8_t buf[7];
  uint8_t a[7];
  uint8_t i;
  for(i=0;i<8;i++){
    a[i] = 0x05;
  }
    
    dInit();

    while(1){
      memset(str,0,sizeof(str));
      memset(buf,0,sizeof(buf));
      DS1307Read(buf,DS1307_CTRL_ID,7);
      
      sprintf(str,"%x %x %x %x %x %x %x\n", (int)buf[0],(int)buf[1],(int)buf[2],
      (int)buf[3], (int)buf[4], (int)buf[5],(int)buf[6]);
      //dLog(str);
      //dLogU(a,7);
      dLogU(buf,7);
      delay_1s();
    }
}