#include "iic.h"

void delay_us(int i)
{
    while(i-->0);
}

void main()
{
    unsigned char rx[4];
    unsigned char tx[] = {0x51, 0x63, 0x8c,0xe4};
    iic_send_str(0xa0, 0x00, tx, 4);
    delay_us(60); // delay between write and read must >= 60us
    iic_receive_str(0xa0, 0x00, rx, 4);
    
    while(1);
}