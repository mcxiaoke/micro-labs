#include <reg51.h>

#define uchar unsigned char
#define uint unsigned int
    
sbit led = P2^0;
uchar num;

void main()
{
    TMOD = 0x01; // timer0 work mode
    TH0 = (65535-45872)/256; // for 11.0592MHz
    TL0 = (65535-45872)%256; // init
    EA = 1; // global on-off
    ET0 = 1; // timer0 interrupt
    TR0 = 1; // start timer0
    while(1) // wait for interrupt
    {
        // 20 * 50ms = 1s
        if(num == 20)
        {
            num = 0;
            led = !led;
        }       
    }
}

void T0_time() interrupt 1
{
    TH0 = (65535-45872)/256; // for 11.0592MHz
    TL0 = (65535-45872)%256; // init
    num++;
}
