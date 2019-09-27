#ifndef _8051_DEBUG_H_
#define _8051_DEBUG_H_

#include <reg51.h>

void UART_Init();
void UART_TxChar(char ch);
char UART_RxChar(void);
void UART_TxString(char* ptr_string);
unsigned char UART_RxString(char* ptr_string);

void dInit();
void dLog(char* ptr_string);

#endif
