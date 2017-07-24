#include<reg52.h>

void ConfigUART(unsigned int baud);

int  main(void)
{
  EA = 1;  //使能总中断
  ConfigUART(9600);  //配置波特率为9600
  while (1);

  return 0;
}
/* 串口配置函数，baud-通信波特率 */
void ConfigUART(unsigned int baud)
{
  SCON = 0x50;  //配置串口为模式1
  TMOD &= 0x0F;  //清零T1的控制位
  TMOD |= 0x20;  //配置T1为模式2
  TH1 = 256 - (11059200 / 12 / 32) / baud; //计算T1重载值
  TL1 = TH1;     //初值等于重载值
  ET1 = 0;       //禁止T1中断
  ES  = 1;      //使能串口中断
  TR1 = 1;       //启动T1
}
/*UART中断服务函数 */
void InterruptUART() interrupt 4
{
  if (RI) //接收到字节
  {
    RI = 0;   //手动清零接收中断标志位
    SBUF = SBUF + 1;  //接收的数据+1后发回，左边是发送SBUF，右边是接收SBUF
  }
  if (TI) //字节发送完毕
  {
    TI = 0;   //手动清零发送中断标志位
  }
}
