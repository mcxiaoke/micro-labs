#include "7seg.h"
#include <reg51.h>

sbit k1 = P2 ^ 0;
sbit k2 = P2 ^ 1;
sbit k3 = P2 ^ 2;
sbit k4 = P2 ^ 3;
sbit LED = P2 ^ 7;

unsigned int num;
unsigned int seconds;
unsigned int temp, d1, d2, d3, d4;

void turn_off() { k1 = k2 = k3 = k4 = 0; }

void delay(unsigned int z)
{
    unsigned int i, j;
    for (i = z; i > 0; i--)
        for (j = 110; j > 0; j--)
            ;
}

void T0_timer() interrupt 1
{
    TH0 = (65536 - 45872) / 256;
    TL0 = (65536 - 45872) % 256;
    // 50ms * 20 = 1sec
    num++;
    if (num == 80) {
        num = 0;
        seconds++;
    }
}

void update_led()
{
    temp = seconds;
    if (temp > 9999) {
        temp = temp % 1000;
    }
    d1 = temp / 1000;
    temp = temp % 1000;
    d2 = temp / 100;
    temp = temp % 100;
    d3 = temp / 10;
    temp = temp % 10;
    d4 = temp;
    turn_off();
    k1 = 1;
    P0 = ANODE[d1];
    delay(10);
    turn_off();
    k2 = 1;
    P0 = ANODE[d2];
    delay(10);
    turn_off();
    k3 = 1;
    P0 = ANODE[d3];
    delay(10);
    turn_off();
    k4 = 1;
    P0 = ANODE[d4];
    delay(10);
}

void main()
{
    turn_off();
    TMOD = 0;
    // 11.0592MHz - 45872
    TH0 = (65536 - 45872) / 256;
    TL0 = (65536 - 45872) % 256;
    EA = 1;
    ET0 = 1;
    TR0 = 1;
    while (1) {
        update_led();
    }
}
