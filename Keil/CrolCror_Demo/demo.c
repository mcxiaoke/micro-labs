#include<reg51.h>

sbit LED27 = P2^3;
sbit LED26 = P2^2;
sbit LED25 = P2^1;
sbit LED24 = P2^0; 

void main(void){
        LED27=1;	
        LED26=1;
        LED25=0;
        LED24=0;
}