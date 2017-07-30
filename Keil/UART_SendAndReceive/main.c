#include "debug.h"
#include "tm1637.h"
#include <stdio.h>

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;

uint16_t count;
uint16_t seconds = 9876;

void reverse(uint8_t *arr, uint16_t length)
{
    uint16_t i,j;
    uint8_t temp;
    j = length - 1;   // j will point to last element
    i = 0;       // i will be pointing to first element
 
    while (i < j) {
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
        i++;             // increment i
        j--;          // decrement j
    }
}

void get_digits(uint16_t num, uint8_t* digits)
{
    uint8_t index = 0;
    uint8_t i = 0;
    while(num > 0){
        i = num % 10;
        num /= 10;
        digits[index++] = i;  
    }
}

void display(uint16_t value)
{
    uint8_t buf[4];
    get_digits(value, buf);
    reverse(buf, 4);
    TM1637_display_numbers(buf,4);
    delay_ms(5);
}

void uart_init()
{
    //TMOD = 0x20;
    TH1 = 0xfd; // timer1 init
    TL1 = 0xfd; // timer1 init
    TR1 = 1; // timer1 start
    SCON = 0x50;
    EA = 1; // global interrupt on
    ES = 1; // serial interrupt on
}

void timer0_init( void )
{
    TMOD = TMOD | 0x01;
    TH0=(65535-45872)/256;
    TL0=(65535-45872)%256;
    ET0=1;
    EA=1;
    TR0=1;
}

void init(){
    timer0_init();
}

void timer0_isr() interrupt 1
{
    TH0=(65535-45872)/256;  //50ms
    TL0=(65535-45872)%256;  //
    count++;
}

void main(){
    debug_init();
    init();
    TM1637_init();
    while(1){
        display(seconds);
        if(count == 20){
            seconds--;
            count = 0;
            debug_printf("Seconds: %u\n", seconds);
            //ES = 0;
            //TI = 1;
            //printf("Seconds: %u\n", seconds);
            //while(!TI);
            //TI = 0;
            //ES = 1;
        }
    }
}