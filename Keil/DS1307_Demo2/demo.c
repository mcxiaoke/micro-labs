#include "Includes.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

void delay_1s()
{
    unsigned int i, j, k;
    for (i = 88; i > 0; i--)
        for (j = 71; j > 0; j--)
            for (k = 40; k > 0; k--)
                ;
}

void read()
{
    char buf[16] = { 0 };
    unsigned char rtc[8];
    unsigned char* d;
    unsigned char* t;
    TI = 1;
    d = Get_DS1307_RTC_Date();
    printf("%d/%d/%d ", (int)d[3], (int)d[2], (int)d[1]);
    memset(buf, 0, sizeof(buf));
    t = Get_DS1307_RTC_Time();
    printf("%d:%d:%d\n", (int)t[2], (int)t[1], (int)t[0]);
}

void main()
{
    dInit();
    dLog("Initialized.\n");
    InitI2C();
    while (1) {
        delay_1s();
        read();
    }
}
