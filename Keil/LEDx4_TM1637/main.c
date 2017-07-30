#include "tm1637.h"

void time_set(int min, int sec);
void time_display( void );
void timer0_init( void );

static unsigned char dpFlag = 0; //控制第二个数码管的dp的显示

unsigned int count = 0;
unsigned int seconds = 0;

/***********************************************************
*****
***** 主函数
*****
***********************************************************/
void main( void )
{
    timer0_init();
    time_set(30, 0);
    TM1637_init();
    while(1)
    {
        time_display();
        dpFlag = (count < 10);
        if(count == 20){
            count = 0;
            if(seconds > 0){
                seconds--;
            }
        }
        delay_ms(10);
    }
}

void time_set(int min, int sec)
{
    seconds = min*60 + sec;
}

/********************************************************************
* 名称 : void time_display( void )
* 功能 : 显示时间
* 输入 : void
* 输出 : 无
**************************************************************/
void time_display( void )
{
    unsigned int min,sec;
    unsigned char mh,ml,sh,sl;
    //seconds = 2601;
    min = seconds/60;
    sec = seconds%60;
    mh = min/10;
    ml = min%10;
    sh = sec/10;
    sl = sec%10;
    
    TM1637_start();
    TM1637_writeCommand(0x44);
    TM1637_writeData(0xc0, CATHODE[mh]);
    //小数点标志为1则用小数点那个数组
    TM1637_writeData(0xc1, (dpFlag||seconds==0)?CATHODE_DOT[ml]:CATHODE[ml]); 
    TM1637_writeData(0xc2, CATHODE[sh]);
    TM1637_writeData(0xc3, CATHODE[sl]);
    TM1637_stop();
}

/********************************************************************
* 名称 : void timer0_init()
* 功能 : 定时50ms，实际运行中由于指令运行造成的延时，实际时间肯定大于50ms
* 输入 : 无
* 输出 : 无
**************************************************************/
void timer0_init( void )
{
    TMOD=0X01;
    TH0=(65535-45872)/256;
    TL0=(65535-45872)%256;
    ET0=1;
    EA=1;
    TR0=1;
}


/********************************************************************
* 名称 :
* 功能 : 定时50ms，实际运行中由于指令运行造成的延时，实际时间肯定大于50ms
* 输入 : 无
* 输出 : 无
**************************************************************/
void timer0_isr() interrupt 1
{
    TH0=(65535-45872)/256;  //50ms
    TL0=(65535-45872)%256;  //
    count++;
}