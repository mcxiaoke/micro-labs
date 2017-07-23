#include "Includes.h"

// Function Purpose: Produce approximate delay in given uSecs.
void __delay_us(unsigned int d)
{
   unsigned int i, limit;
   limit = d/15;

   for(i=0;i<limit;i++);
}


// Function Purpose: Set initial values of SCK and SDA pins
void InitI2C(void)
{	
	// Make SDA and SCK pins input initially
	SDA = 1;
	SCK = 1;
}


// Function Purpose: I2C_Start sends start bit sequence
void I2C_Start(void)
{
	Set_SCK_High;				// Make SCK pin high
	Set_SDA_High;				// Make SDA pin High
	__delay_us(HalfBitDelay);	// Half bit delay
	Set_SDA_Low;				// Make SDA Low
	__delay_us(HalfBitDelay);	// Half bit delay
}


// Function Purpose: I2C_ReStart sends start bit sequence
void I2C_ReStart(void)
{
	Set_SCK_Low;				// Make SCK pin low

	__delay_us(HalfBitDelay/2);	// Data pin should change it's value,
								// when it is confirm that SCK is low
	Set_SDA_High;				// Make SDA pin High
	
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
	Set_SCK_High;				// Make SCK pin high
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
	Set_SDA_Low;				// Make SDA Low
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
}


//Function : I2C_Stop sends stop bit sequence
void I2C_Stop(void)
{
	Set_SCK_Low;				// Make SCK pin low

	__delay_us(HalfBitDelay/2);	// Data pin should change it's value,
								// when it is confirm that SCK is low
	Set_SDA_Low;				// Make SDA pin low
	
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
	Set_SCK_High;				// Make SCK pin high
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
	Set_SDA_High;				// Make SDA high
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
}



//Function : I2C_Send_ACK sends ACK bit sequence
void I2C_Send_ACK(void)
{
	Set_SCK_Low;				// Make SCK pin low
	__delay_us(HalfBitDelay/2);	// Data pin should change it's value,
								// when it is confirm that SCK is low
	Set_SDA_Low;				// Make SDA Low
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
	Set_SCK_High;				// Make SCK pin high
	__delay_us(HalfBitDelay);	// Half bit delay
}


//Function : I2C_Send_NACK sends NACK bit sequence
void I2C_Send_NACK(void)
{
	Set_SCK_Low;				// Make SCK pin low
	__delay_us(HalfBitDelay/2);	// Data pin should change it's value,
								// when it is confirm that SCK is low
	Set_SDA_High;				// Make SDA high
	__delay_us(HalfBitDelay/2);	// 1/4 bit delay
	Set_SCK_High;				// Make SCK pin high
	__delay_us(HalfBitDelay);	// Half bit delay
}


// Function Purpose: I2C_Write_Byte transfers one byte
bit I2C_Write_Byte(unsigned char Byte)
{
	unsigned char i;		// Variable to be used in for loop
	
	for(i=0;i<8;i++)		// Repeat for every bit
	{
		Set_SCK_Low;		// Make SCK pin low
		
		__delay_us(HalfBitDelay/2);	// Data pin should change it's value,
									// when it is confirm that SCK is low

		if((Byte<<i)&0x80)  // Place data bit value on SDA pin
			Set_SDA_High;	// If bit is high, make SDA high
		else				// Data is transferred MSB first
			Set_SDA_Low;	// If bit is low, make SDA low

		__delay_us(HalfBitDelay/2);	// Toggle SCK pin
		Set_SCK_High;				// So that slave can
		__delay_us(HalfBitDelay);	// latch data bit
    }
		
	// Get ACK from slave
	Set_SCK_Low;
    Set_SDA_High;
    __delay_us(HalfBitDelay);
    Set_SCK_High;
    __delay_us(HalfBitDelay);

	return SDA;
}


// Function Purpose: I2C_Read_Byte reads one byte
unsigned char I2C_Read_Byte(void)
{
	unsigned char i, d, RxData = 0;

	for(i=0;i<8;i++)
	{
		Set_SCK_Low;					// Make SCK pin low
		Set_SDA_High;					// Don't drive SDA 
		__delay_us(HalfBitDelay);		// Half bit delay
		Set_SCK_High;					// Make SCK pin high
		__delay_us(HalfBitDelay/2);		// 1/4 bit delay
		d = SDA;					    // Capture Received Bit
		RxData = RxData|(d<<(7-i));   	// Copy it in RxData
		__delay_us(HalfBitDelay/2);		// 1/4 bit delay
	}
 
    return RxData;						// Return received byte
}