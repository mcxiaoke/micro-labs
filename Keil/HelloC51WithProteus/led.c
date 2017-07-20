#include<reg51.h>

sbit LED27 = P2^7;
sbit LED26 = P2^6;
sbit LED25 = P2^5;
sbit LED24 = P2^4;

// for 12MHz
void delay_1s(){
	unsigned char i,j,k;
	for(i = 98;i > 0; i--)
	for(j = 101; j > 0; j--)
	for(k = 49; k > 0; k--);
} 

void delay_500ms(){
	unsigned char i,j,k;
	for(i = 68;i > 0; i--)
	for(j = 61; j > 0; j--)
	for(k = 29; k > 0; k--);
} 

void main(void){
	while(1){    
        LED27=!LED27;
        delay_500ms();	
        LED26=!LED26;
        delay_500ms();	
        LED25=!LED25;
        delay_500ms();	
        LED24=!LED24;
        delay_500ms();
    }
}