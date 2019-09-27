#include <reg51.h>

sbit LED = P2^0;

void delay_1s(){
	unsigned int i,j,k;
	for(i = 60;i > 0; i--)
	for(j = 80; j > 0; j--)
	for(k = 40; k > 0; k--);
}

unsigned int num;
void main(){
  LED = 1;
  while(1){
    LED = ~LED;
    delay_1s();
  }
}