#ifndef _8051_DEBUG_H_
#define _8051_DEBUG_H_

#include <reg51.h>

typedef signed char int8_t;
typedef signed char sint8_t;
typedef unsigned char uint8_t;

void UART_Init();
void UART_TxChar(char ch);
char UART_RxChar(void);
void UART_TxString(char* ptr_string);
uint8_t UART_RxString(char* ptr_string);

void dInit();
void dLog(char* ptr_string);

#endif
