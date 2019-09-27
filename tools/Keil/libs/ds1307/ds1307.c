#include "ds1307.h"

static unsigned char bcd2bin(unsigned char val) { return val - 6 * (val >> 4); }
static unsigned char bin2bcd(unsigned char val) { return val + 6 * (val / 10); }

static void fill_buffer(RTC *rtc)
{
    rtc->buf[0] = bin2bcd(rtc->seconds);
    rtc->buf[1] = bin2bcd(rtc->minutes);
    rtc->buf[2] = bin2bcd(rtc->hours);
    rtc->buf[3] = bin2bcd(rtc->days);
    rtc->buf[4] = bin2bcd(rtc->date);
    rtc->buf[5] = bin2bcd(rtc->month);
    rtc->buf[6] = bin2bcd(rtc->year);
}

void ds1307_delay_ms(int i)
{
    int k;
    for(;i>0;i--)
        for(k=110;k>0;k--);
}

void ds1307_init()
{
    i2c_init();
    i2c_start();
    i2c_write(DS1307_MODE_WRITE);
    i2c_write(DS1307_REG_ADDR_CTRL);
    i2c_write(0x00);//RS1 = RS0 = 0
    i2c_stop();
}

void ds1307_read(RTC *rtc)
{
    unsigned char temp;
    _ds1307_read_raw(DS1307_REG_ADDR_SEC, rtc->buf, DS1307_LENGTH);
    rtc->seconds = bcd2bin(rtc->buf[0]);
    rtc->minutes = bcd2bin(rtc->buf[1]);
    temp = rtc->buf[2];
    if(temp & 0x40){
       // 12 hours mode 
        rtc->mode = MODE_12HOURS;
       if(temp & 0x20){
          rtc->hours = bcd2bin(temp)+12;  
       } else{
          rtc->hours = bcd2bin(temp);
       }
    }else{
        rtc->mode = MODE_24HOURS;
        rtc->hours = bcd2bin(temp);
    }
    rtc->days = bcd2bin(rtc->buf[3]);
    rtc->date = bcd2bin(rtc->buf[4]);
    rtc->month = bcd2bin(rtc->buf[5]);
    rtc->year = bcd2bin(rtc->buf[6]);
}

bit ds1307_write(RTC *rtc)
{
    fill_buffer(rtc);
    return _ds1307_write_raw(DS1307_REG_ADDR_SEC, rtc->buf,DS1307_LENGTH);
}

void ds1307_read_rom(unsigned char buf[], int len)
{
    _ds1307_read_raw(DS1307_REG_ADDR_ROM, buf, len);
}

bit ds1307_write_rom(unsigned char buf[], int len)
{
    return _ds1307_write_raw(DS1307_REG_ADDR_ROM, buf ,len);
}

void _ds1307_read_raw(unsigned char addr, unsigned char buf[], int len)
{
    unsigned char i;
    i2c_start();
    while(i2c_write(DS1307_MODE_WRITE)==ERR){
        i2c_start();
    }
    i2c_write(addr);
    i2c_start();
    i2c_write(DS1307_MODE_READ);
    for(i=0;i<len;i++){
        buf[i] = i2c_read();
        if(i < len -1){
            i2c_ack();
        }
    }
    i2c_noack();
    i2c_stop();  
}

bit _ds1307_write_raw(unsigned char addr, unsigned char buf[], int len)
{
    unsigned char i;
    i2c_start();
    while(i2c_write(DS1307_MODE_WRITE)==ERR){
        i2c_start();
    }
    if(i2c_write(addr) == ERR){
        return ERR;
    }
    for(i=0;i<len;i++){
        if(i2c_write(buf[i]) == ERR){
            return ERR;
        }
    }
    i2c_stop();  
    return SUCC;
}