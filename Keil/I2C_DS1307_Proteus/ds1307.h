#ifndef _DS1307_H_
#define _DS1307_H_

#include "../libs/i2c/i2c.h"

#define DS1307_DEVICE_ID 0xd0
#define DS1307_MODE_WRITE 0xd0
#define DS1307_MODE_READ 0xd1

#define DS1307_REG_ADDR_SEC 0x00
#define DS1307_REG_ADDR_CTRL 0x07
#define DS1307_REG_ADDR_ROM 0x08
#define DS1307_REG_ADDR_ROM_MAX 0x3f

#define DS1307_LENGTH 7
#define MODE_24HOURS 0 
#define MODE_12HOURS 1

typedef struct {
    unsigned char buf[DS1307_LENGTH]; // raw bcd buffer
    unsigned char seconds; // 0-59
    unsigned char minutes; // 0-59
    unsigned char hours; // am/pm 0-11 or 0-23
    unsigned char days; // week days 1-7 1=sunday
    unsigned char date; // days 1-31
    unsigned char month; // 1-12
    unsigned char year; // 0-99 (+ 2000)
    unsigned char mode; // 12-1, 24-0
} RTC;

void ds1307_delay_ms(int i);
void ds1307_init();
void ds1307_read(RTC *rtc);
bit ds1307_write(RTC *rtc);
void ds1307_read_rom(unsigned char buf[], int len);
bit ds1307_write_rom(unsigned char buf[], int len);

void _ds1307_read_raw(unsigned char addr, unsigned char buf[], int len);
bit _ds1307_write_raw(unsigned char addr, unsigned char buf[], int len);

#endif