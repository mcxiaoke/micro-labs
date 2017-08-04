#include "../libs/i2c/i2c.h"

void delay_us(int i)
{
    while(i-->0);
}

void main()
{
    unsigned char rx[8];
    unsigned char tx[] = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0};
    i2c_write_str(0xa0, 0x00, tx, 8);
    delay_us(60); // delay between write and read must >= 60us
    i2c_read_str(0xa0, 0x00, rx, 8);
    
    while(1);
}