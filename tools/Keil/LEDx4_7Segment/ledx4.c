#include <reg51.h>
#include "7seg.h"

sbit k1 = P2^0;
sbit k2 = P2^1;
sbit k3 = P2^2;
sbit k4 = P2^3;

void turn_off(){
    k1 = k2 = k3 = k4 = 0;
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
    int i = 1234;
    int temp = i;
    while (1) {
      // 4
      turn_off();
      k4 = 1;
      P0 = ANODE[3];
      delay(5);
      // 3
      turn_off();
      k3 = 1;
      P0 = ANODE[2];
      delay(5);
      // 2
      turn_off();
      k2 = 1;
      P0 = ANODE[1];
      delay(5);
      //1
      turn_off();
      k1 = 1;
      P0 = ANODE[0];
      delay(5);
    }
}















