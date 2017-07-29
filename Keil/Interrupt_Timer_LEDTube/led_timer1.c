#include    <reg51.h>
#include    <intrins.h>
#include    <limits.h>
#include	"tm1638.h"

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long

uchar num;
ulong count = 65535;
   
void get_digits(unsigned long num, unsigned char* digits)
{
    uchar index = 0;
    uchar i = 0;
    while(num > 0){
        i = num % 10;
        num /= 10;
        digits[index++] = i;  
    }
}

void delay(int ms)
{
    int i, j;
    for(i = 0;i<ms;i++)
        for(j=0;j<100;j++)
            ;
}

void init()
{
	//unsigned char i;
	init_TM1638(); // init TM1638
	//for(i=0;i<8;i++)
	//Write_DATA(i<<1,tab[0]);
}

void update_led2()
{
    ulong tmp;
    uchar digits[8] = {0};
    
    uchar i;
    if(count >= 100000000){
        tmp = count % 100000000;
    }else{
        tmp = count;
    }
    get_digits(tmp, digits);
    for(i = 7;i>=0;i--){
        Write_DATA(i<<1, tab[digits[i]]);
    }
}

void update_led()
{
    uint tmp = count;
    Write_DATA(0<<1, tab[0]);
    Write_DATA(1<<1, tab[0]);
    Write_DATA(2<<1, tab[0]);
    // 16bit max = 65535
    Write_DATA(3<<1, tab[tmp/10000]);
    tmp = tmp % 10000;
    Write_DATA(4<<1, tab[tmp/1000]);
    tmp = tmp % 1000;
    Write_DATA(5<<1, tab[tmp/100]);
    tmp = tmp % 100;
    Write_DATA(6<<1, tab[tmp/10]);
    Write_DATA(7<<1, tab[tmp%10]);
}

void main()
{
    init();
    update_led();
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
            count--;
            update_led();
        }
    }
}

void T0_time() interrupt 1
{
    TH0 = (65535-45872)/256; // for 11.0592MHz
    TL0 = (65535-45872)%256; // init
    num++;
}
