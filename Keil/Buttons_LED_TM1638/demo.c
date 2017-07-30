#include    <intrins.h>
#include    <limits.h>
#include	"tm1638.h"

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long

uchar num;
ulong count = 0;
   
void get_digits(unsigned long num, unsigned char* digits)
{
    uchar index = 0;
    uchar i = 0;
    while(num > 0){
        i = (uchar)(num % 10);
        num /= 10;
        digits[index++] = i;  
    }
}

ulong pow(ulong base, uchar exp)
{
    ulong i = 1;
    while(exp-->0){
        i = i * base;
    }
    return i;
}

void delay(int ms)
{
    int i, j;
    for(i = 0;i<ms;i++)
        for(j=0;j<100;j++)
            _nop_();
}

void init_ic()
{
	unsigned char i;
	init_TM1638(); // init TM1638
	for(i=0;i<8;i++)
        Write_DATA(i<<1,tab[0]);
}

void show_numbers()
{
    ulong tmp = count;
    // 32bit num max 10 digits
    uchar digits[10];
    uchar i;
    get_digits(tmp, digits);
    // 8 digital led
    for(i = 0;i<8;i++){
        // right->left, d1-d8
        Write_DATA((7-i)<<1, tab[digits[i]]);
    }
}

void init_timer()
{
    TMOD = 0x01; // timer0 work mode
    TH0 = (65535-45872)/256; // for 11.0592MHz
    TL0 = (65535-45872)%256; // init
    EA = 1; // global on-off
    ET0 = 1; // timer0 interrupt
    TR0 = 1; // start timer0
}

void T0_time() interrupt 1
{
    TH0 = (65535-45872)/256; // for 11.0592MHz
    TL0 = (65535-45872)%256; // init
    num++;
}

void main()
{
    unsigned char i;
    init_timer();
    init_ic();
    show_numbers();
    while(1) // wait for interrupt
    {
        i = Read_key();
        if(i<8){
            count += pow(10,7-i);
            while(Read_key() == i);
            show_numbers();
        }
        // 20 * 50ms = 1s
        if(num == 20)
        {
            num = 0;
            count++;
            show_numbers();
        }
    }
}