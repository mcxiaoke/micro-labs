#include "at24c.h"

void I2CWait()
{
  unsigned char i;
  while((SCL==0)&&(++i<250));
}

void I2CDelay()
{
  _nop_();
  _nop_();
  _nop_();
  _nop_();
}
 
void I2CInit()
{
	SDA = 1;
  I2CDelay();
	SCL = 1;
  I2CDelay();
}
 
void I2CStart()
{
	SDA = 0;
  I2CDelay();
	SCL = 0;
  I2CDelay();
}
 
void I2CRestart()
{
	SDA = 1;
  I2CDelay();
	SCL = 1;
  I2CDelay();
	SDA = 0;
  I2CDelay();
	SCL = 0;
  I2CDelay();
}
 
void I2CStop()
{
	SCL = 0;
  I2CDelay();
	SDA = 0;
  I2CDelay();
	SCL = 1;
  I2CDelay();
	SDA = 1;
  I2CDelay();
}
 
void I2CAck()
{
	SDA = 0;
  I2CDelay();
	SCL = 1;
  I2CDelay();
	SCL = 0;
  I2CDelay();
	SDA = 1;
  I2CDelay();
}
 
void I2CNak()
{
	SDA = 1;
  I2CDelay();
	SCL = 1;
  I2CDelay();
	SCL = 0;
  I2CDelay();
	SDA = 1;
  I2CDelay();
}
 
unsigned char I2CSend(unsigned char Data)
{
	 unsigned char i, ack_bit;
	 for (i = 0; i < 8; i++) {
		if ((Data & 0x80) == 0)
			SDA = 0;
		else
			SDA = 1;
    I2CDelay();
		SCL = 1;
    I2CDelay();
	 	SCL = 0;
    I2CDelay();
		Data<<=1;
	 }
	 SDA = 1;
   I2CDelay();
	 SCL = 1;
   I2CDelay();
	 ack_bit = SDA;
	 SCL = 0;
   I2CDelay();
	 return ack_bit;
}
 
unsigned char I2CRead()
{
	unsigned char i, Data=0;
	for (i = 0; i < 8; i++) {
		SCL = 1;
    I2CDelay();
		if(SDA)
			Data |=1;
		if(i<7)
			Data<<=1;
		SCL = 0;
    I2CDelay();
	}
	return Data;
}