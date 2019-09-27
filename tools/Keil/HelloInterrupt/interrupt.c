#include <reg51.h>

#define uchar unsigned char
#define uint unsigned int
    
sbit LED27 = P2^7;
sbit LED26 = P2^6;
sbit LED25 = P2^5;
sbit LED24 = P2^4;

uchar num;

void main(){
    LED27 = 0x01;
    LED26 = 0x01;
    LED25 = 0x01;
    LED24 = 0x01;
    TMOD = 0;
    // 11.0592MHz - 45872
    TH0 = (65536-45872)/256;
    TL0 = (65536-45872)%256;
    EA = 1;
    ET0 = 1;
    TR0 = 1;
    while(1){
        if (num == 20*8){
            LED27 = ~LED27;
            num = 0;
        } else if (num == 20*6){
            LED26 = ~LED26;
        } else if (num == 20*4){
            LED25 = ~LED25;
        } else if (num == 20*2){
            LED24 = ~LED24;
        }
    }
}

void T0_timer() interrupt 1{
    TH0 = (65536-45872)/256;
    TL0 = (65536-45872)%256;
    // 50ms * 20 = 1sec
    num++;
}