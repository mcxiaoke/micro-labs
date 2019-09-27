#include "debug.h"
#include "rtc.h"
#include "stdio.h"

void delay_1s()
{
    unsigned int i, j, k;
    for (i = 98; i > 0; i--)
        for (j = 101; j > 0; j--)
            for (k = 49; k > 0; k--)
                ;
}

// Convert normal decimal numbers to binary coded decimal
unsigned char decToBcd(unsigned char val)
{
    return ((val / 10 * 16) + (val % 10));
}

// Convert binary coded decimal to normal decimal numbers
unsigned char bcdToDec(unsigned char val)
{
    return ((val / 16 * 10) + (val % 16));
}

unsigned char bcd2bin(unsigned char val) { return val - 6 * (val >> 4); }
unsigned char bin2bcd(unsigned char val) { return val + 6 * (val / 10); }

// #define DS1307_ADDRESS              (0x68)

// #define DS1307_REG_TIME             (0x00)
// #define DS1307_REG_CONTROL          (0x07)
// #define DS1307_REG_RAM              (0x08)

/**
Slave Address

7-Bit format: 0b1101000 = 0x68
Slave address for I2C Write: 0b11010000 = 0xD0
Slave address for I2C Read: 0b11010001 = 0xD1
**/

void read()
{
    unsigned char buf[16];
    unsigned char sec, min, hour;
    unsigned char weekDay, date, month, year;
    rtc_t rtc;
    RTC_Init();
    RTC_GetDateTime(&rtc);

    sec = bcd2bin(rtc.sec);
    min = bcd2bin(rtc.min);
    hour = bcd2bin(rtc.hour);
    weekDay = bcd2bin(rtc.weekDay);
    date = bcd2bin(rtc.date);
    month = bcd2bin(rtc.month);
    year = bcd2bin(rtc.year);

    // n = sprintf(buf, "17/07/24 ");
    // n = sprintf(buf+n, "13:20:30\n");
    // n = sprintf(buf, "%x/%x/%x ", 0x17u, 0x07u, 0x24u);
    // n = sprintf(buf+n, "%x:%x:%x\n", 0x13u, 0x20u, 0x30u);
    sprintf(buf, "%x/%x/%x,", rtc.year, rtc.month, rtc.date);
    dLog(buf);
    sprintf(buf, "%x:%x:%x\n", rtc.hour, rtc.min, rtc.sec);
    dLog(buf);
}

void main()
{
    dInit();
    delay_1s();
    while (1) {
        read();
        delay_1s();
    }
}
