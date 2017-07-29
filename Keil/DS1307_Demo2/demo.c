#include "debug.h"
#include "i2c_a.h"
#include <stdio.h>
#include <string.h>

void delay_1s()
{
    unsigned int i, j, k;
    for (i = 100; i > 0; i--)
        for (j = 100; j > 0; j--)
            for (k = 60; k > 0; k--)
                ;
}


unsigned char a1,a2,a3;
unsigned char a4,a5,a6,a7;
char buf[64];
void read()
{
    dLog("Begin Read:");
    InitI2c();
    a1 = read_byte(0x0);
    a2 = read_byte(0x1);
    a3 = read_byte(0x2);
    a4 = read_byte(0x3);
    a5 = read_byte(0x4);
    a6 = read_byte(0x5);
    a7 = read_byte(0x6);
    memset(buf,0,sizeof(buf));
    sprintf(buf, "%x %x %x %x %x %x %x",
        (int)1,(int)2,(int)3,
        (int)4,(int)5,(int)6,(int)7);
    dLog(buf);
    dLog(":End Read");
}

void main()
{
    dInit();
    dLog("Hello,World!\n");
    while (1) {
        read();
        delay_1s();
    }
}
