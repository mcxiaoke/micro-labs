#ifndef __DEBUG_HEADER__
#define __DEBUG_HEADER__

#include <reg51.h>
#include <stdio.h>
#include <stdarg.h>

void debug_init();
void debug_print(unsigned char* message);
void debug_printf(unsigned char* format, ...);

#endif

