#include "i2c.h"

#define _nop4_  {_nop_();_nop_();_nop_();_nop_();}

void delay_us(int i){
    while(i--);
}

void delay_ms(int i){
    unsigned char j;
    for(;i>0;i--)
        for(j=110;j>0;j--);
}

void i2c_init(){
    SDA = 1;
    SCL = 1;
}

void i2c_start(){
    SDA = 1;
    SCL = 1;
    _nop4_;
    SDA = 0;
    _nop4_;
    SCL = 0;
    _nop4_;
}

void i2c_stop(){
    SDA = 0;
    SCL = 0;
    _nop4_;
    SCL = 1;
    _nop4_;
    SDA = 1;
    _nop4_;
}

void i2c_ack(bit ack){
    SDA = ack;
    _nop4_;
    SCL = 1;
    _nop4_;
    SCL = 0;    
}

bit i2c_write(unsigned char dat){
    bit ack;
    unsigned char mask;
    for(mask = 0x80;mask!=0;mask>>=1){
        if((mask&dat)==0){
            SDA = 0;
        }else{
            SDA = 1;
        }
        _nop4_;
        SCL = 1;
        _nop4_;
        SCL = 0;
    }
    SDA = 1;
    _nop4_;
    SCL = 1;
    ack = SDA;
    _nop4_;
    SCL = 0;
    return ~ack;
}


unsigned char i2c_read_nack(){
    unsigned char mask;
    unsigned char dat;
    SDA = 1;
    for(mask=0x80;mask!=0;mask>>=1){
        _nop4_;
        SCL = 1;
        if(SDA==0){
            dat &= ~mask;
        }else{
            dat |= mask;
        }
        _nop4_;
        SCL = 0;
    }
    SDA = 1;
    _nop4_;
    SCL = 1;
    _nop4_;
    SCL = 0;
    return dat;
}

unsigned char i2c_read_ack(){
    unsigned char mask;
    unsigned char dat;
    SDA = 1;
    for(mask=0x80;mask!=0;mask>>=1){
        _nop4_;
        SCL = 1;
        if(SDA==0){
            dat &= ~mask;
        }else{
            dat |= mask;
        }
        _nop4_;
        SCL = 0;
    }
    SDA = 0;
    _nop4_;
    SCL = 1;
    _nop4_;
    SCL = 0;
    return dat;  
}