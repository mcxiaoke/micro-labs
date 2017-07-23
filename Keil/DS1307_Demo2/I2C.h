
#ifndef __I2C_H
#define __I2C_H

// Define i2c pins
sbit SDA = P0^6;		// Data pin for i2c
sbit SCK = P0^7;		// Clock pin for i2c

// Define i2c speed
#define I2C_SPEED_FACTOR	1	// Low value means low i2c frequency
#define Crystal_Value		12  // MHz
#define HalfBitDelay 		(500*Crystal_Value)/(12*I2C_SPEED_FACTOR)

// Define macros
#define Set_SDA_Low		SDA = 0
#define Set_SDA_High	SDA = 1
#define Set_SCK_Low		SCK = 0
#define Set_SCK_High	SCK = 1

//Function Declarations
void __delay_us(unsigned int);
void InitI2C(void);
void I2C_Start(void);
void I2C_ReStart(void);
void I2C_Stop(void);
void I2C_Send_ACK(void);
void I2C_Send_NACK(void);
bit  I2C_Write_Byte(unsigned char);
unsigned char I2C_Read_Byte(void);

#endif
