#include "iic.h"

void main()
{
  //iic_start();
  //iic_send_byte(0xa0);
  //iic_send_byte(0x04);
  //iic_send_byte(0x08);
  //iic_stop();
  unsigned char tmp[2];
  //iic_send_str(0xa0, 0x04, 0x08, 0);
  //iic_send_str(0xa0, 0x05, 0x09, 1);
  iic_receive_str(0xa0,0x04,tmp,2);
  
  while(1);
}