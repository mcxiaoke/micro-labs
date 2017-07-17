#include<reg51.h>

sbit LED = P2^7;

// for 12MHz
void delay_1s(){
	unsigned char i,j,k;
	for(i = 98;i > 0; i--)
	for(j = 101; j > 0; j--)
	for(k = 49; k > 0; k--);
}

void main(void){
	while(1){
		LED = !LED;	
		delay_1s();		
	}
}