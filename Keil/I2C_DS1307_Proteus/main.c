#include "../libs/ds1307/ds1307.h"

#define ADDR 0xd0

void test_rtc()
{
    RTC rtc;
    rtc.seconds = 45;
    rtc.minutes = 30;
    rtc.hours = 15;
    rtc.days = 6;
    rtc.date = 28;
    rtc.month = 7;
    rtc.year = 17;
    rtc.mode = MODE_24HOURS;
    //ds1307_delay_ms(500);
    ds1307_init();
    ds1307_write(&rtc);
    ds1307_read(&rtc);
}

void  test_rom()
{
    unsigned char temp[16]; 
    unsigned char buf[] = "Hello, World";
    ds1307_read_rom(temp, 16);
    //ds1307_write_rom(buf,sizeof(buf)/sizeof(unsigned char));
}

void main()
{ 
    test_rtc();
    while(1);
}