#include <reg51.h>

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;

static uint8_t flag,a,i;
static uint8_t code table[] = "I get ";

static void uart_init()
{
    TMOD = 0x20; //set timer1 mode 2
    TH1 = 0xfd; // timer1 init
    TL1 = 0xfd; // timer1 init
    TR1 = 1; // timer1 start
    REN = 1; // allow serial receive
    SM0 = 0; // serial mode 1
    SM1 = 1; // serial mode 1
    EA = 1; // global interrupt on
    ES = 1; // serial interrupt on
}

static void uart_receive() interrupt 4
{
    RI = 0; // wait next receive
    a = SBUF; // read 
    flag = 1; // received flag
}


static void main0()
{
    uart_init();
    while(1){
        if(flag == 1){
            ES = 0; // serial int off
            for(i=0;i<6;i++){
                SBUF = table[i];
                while(!TI); // wait send complete
                TI = 0;
            }
            SBUF = a; // send data
            while(!TI); // wait send complete
            TI = 0;
            SBUF = '\n';
            while(!TI);
            TI = 0;
            ES = 1; // serial int on
            flag = 0;
        }
    }
    
}