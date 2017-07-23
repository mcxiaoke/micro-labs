#include "debug.h"
#include <reg51.h>

void UART_Init()
{
    SCON = 0x50; /* uart in mode 1 (8 bit), REN=1 */
    TMOD = TMOD | 0x20; /* Timer 1 in mode 2 */
    TH1 = 0xFD; /* 9600 Bds at 11.059MHz */
    TL1 = 0xFD; /* 9600 Bds at 11.059MHz */
    ES = 1; /* Enable serial interrupt */
    EA = 1; /* Enable global interrupt */
    TR1 = 1; /* Timer 1 run */
}

void UART_TxChar(char ch)
{
    SBUF = ch; // Load the data to be transmitted
    while (TI == 0)
        ; // Wait till the data is trasmitted
    TI = 0; //Clear the Tx flag for next cycle.
}

char UART_RxChar(void)
{
    while (RI == 0)
        ; // Wait till the data is received
    RI = 0; // Clear Receive Interrupt Flag for next cycle
    return (SBUF); // return the received char
}

void UART_TxString(char* ptr_string)
{
    while (*ptr_string)
        UART_TxChar(*ptr_string++); // Loop through the string and transmit char by char
}

unsigned char UART_RxString(char* ptr_string)
{
    char ch;
    unsigned char len = 0;
    while (1) {
        ch = UART_RxChar(); //Receive a char
        UART_TxChar(ch); //Echo back the received char

        if ((ch == '\r') || (ch == '\n')) //read till enter key is pressed
        { //once enter key is pressed null terminate the string
            ptr_string[len] = 0; //and break the loop
            break;
        } else if ((ch == '\b') && (len != 0)) {
            len--; //If backspace is pressed then decrement the index to remove the old char
        } else {
            ptr_string[len] = ch; //copy the char into string and increment the index
            len++;
        }
    }
    return len;
}

void dInit()
{
    UART_Init();
}

void dLog(char* ptr_string)
{
    UART_TxString(ptr_string);
}
