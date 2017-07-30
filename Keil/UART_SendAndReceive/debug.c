#include "debug.h"

void debug_init()
{
    TMOD = 0x20;
    TH1 = 0xfd; // timer1 init
    TL1 = 0xfd; // timer1 init
    TR1 = 1; // timer1 start
    SCON = 0x50;
    EA = 1; // global interrupt on
    ES = 1; // serial interrupt on
}

void debug_print(unsigned char* message)
{
    ES = 0;
    TI = 1;
    puts(message);
    while(!TI);
    TI = 0;
    ES = 1;
}

void debug_printf(unsigned char* format, ...)
{
    va_list vl;
    va_start(vl,format);
    ES = 0;
    TI = 1;
    vprintf(format, vl);
    while(!TI);
    TI = 0;
    ES = 1;
    va_end(vl);
}