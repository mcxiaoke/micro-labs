#include <reg52.h>
#include "7seg.h"

// upside l->r
sbit k1 = P1^5;//1 12
sbit SA = P1^4;//a 11
sbit SF = P1^3;//f 10
sbit k2 = P1^2;//2 9
sbit k3 = P1^1;//3 8
sbit SB = P1^0;//b 7

// downside l->r
sbit SE = P2^0;//e 1
sbit SD = P2^1;//d 2
sbit SZ = P2^2;//dp 3
sbit SC = P2^3;//c 4
sbit SG = P2^4;//g 5
sbit k4 = P2^5;//4 6

void turn_off(){
    k1 = k2 = k3 = k4 = 1;
}

void show_num(unsigned char num){
  unsigned char c = CATHODE[num];
  SA = (c >> 0) & 0x01;
  SB = (c >> 1) & 0x01;
  SC = (c >> 2) & 0x01;
  SD = (c >> 3) & 0x01;
  SE = (c >> 4) & 0x01;
  SF = (c >> 5) & 0x01;
  SG = (c >> 6) & 0x01;
  SZ = (c >> 7) & 0x01;

}

void show_all(unsigned char num){
  SA = SB = SC = SD = SE = SF = SG = 0;
  SZ = 1;
}

void delay(unsigned int z)
{
  unsigned int i,j;
  for(i=z;i>0;i--)
    for (j = 110; j > 0; j--)
      ;
}

void main()
{
    while (1) {

      // 1 left high
      turn_off();
      k1 = 0;
      show_num(4);
      delay(5);

      // 2
      turn_off();
      k2 = 0;
      show_num(3);
      delay(5);

      // 3
      turn_off();
      k3 = 0;
      show_num(2);
      delay(5);

      //4 right low
      turn_off();
      k4 = 0;
      show_num(1);
      delay(5);
    }
}















