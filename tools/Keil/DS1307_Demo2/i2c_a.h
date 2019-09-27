#include <reg51.h>


//Delay for I2c
#define I2C_DELAY    50

//Define Led Toggle Time
#define TOGGLE_LED  20000

//control address of ds1307
#define device_addr 0x68

#define ACK_BIT    0


//Define the Pin for the I2c and lec
sbit SDA_BUS = P2^0;
sbit SCL_BUS = P2^1;
sbit Led = P3^0;

/*=========================================
Prototypes for I2c functions 
==========================================*/


void InitI2c(void);

void StartI2c(void);

void RepeatedStartI2c(void);

void StopI2c(void);

void SendAckBit(void);

void SendNackBit(void);

void delay(unsigned int);

bit write_i2c(unsigned char);

unsigned char read_i2c(void);

void write_byte(unsigned int ,unsigned char);

unsigned char read_byte(unsigned int);

