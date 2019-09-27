/*
* @Author: mcxiaoke
* @Date:   2017-07-22 23:51:10
* @Last Modified by:   mcxiaoke
* @Last Modified time: 2017-07-23 00:05:35
*/

#include "debug.h"

char ch;

void main()
{
    char* str = "Welcome to 8051 Serial Comm, Type the char to be echoed:";
    dInit(); //Initialize the UART module with 9600 baud rate
    dLog(str);
    while (1) {
        ch = UART_RxChar(); // Receive a char from serial port
        UART_TxChar(ch); // Transmit the received char
    }
}
