#include <reg51.h>

unsigned char code num[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e};

void delay_ms(unsigned int z)   
{
  unsigned int i,j;
  for(i=z;i>0;i--)
     for(j=110;j>0;j--);        
}

void main()
{
    unsigned int i;
    while(1)
    {
        for(i=0;i<16;i++)
        {
           P2=num[i];
           delay_ms(500);
           P2=0xff;
           delay_ms(100);
        }
    }
}