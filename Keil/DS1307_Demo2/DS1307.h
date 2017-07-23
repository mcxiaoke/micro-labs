
#ifndef __DS1307_H
#define __DS1307_H


// Define DS1307 i2c device address
#define Device_Address_DS1307_EEPROM	0xD0

// Define Time Modes
#define AM_Time					0
#define PM_Time					1
#define TwentyFourHoursMode		2

// Define days
#define Monday		1
#define Tuesday		2
#define Wednesday	3	
#define Thursday	4
#define Friday		5
#define Saturday	6
#define Sunday		7

// Function Declarations
void delay(unsigned int);
void Write_Byte_To_DS1307_RTC(unsigned char, unsigned char);
unsigned char Read_Byte_From_DS1307_RTC(unsigned char);
void Write_Bytes_To_DS1307_RTC(unsigned char,unsigned char*,unsigned char);
void Read_Bytes_From_DS1307_RTC(unsigned char,unsigned char*,unsigned int);
void Set_DS1307_RTC_Time(unsigned char,unsigned char,unsigned char,unsigned char);
unsigned char* Get_DS1307_RTC_Time(void);
void Set_DS1307_RTC_Date(unsigned char,unsigned char,unsigned char,unsigned char);
unsigned char* Get_DS1307_RTC_Date(void);

#endif