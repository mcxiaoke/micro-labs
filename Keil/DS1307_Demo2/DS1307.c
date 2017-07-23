#include "Includes.h"

// Global RTC Array and temp variable
unsigned char pRTCArray[4];
unsigned char Temp;

   
// Function Purpose: delay generate some delay according to d value  
void delay(unsigned int d)
{
	unsigned int i;
	for(i=0;i<d;i++);
}



// Function Purpose: Write_Byte_To_DS1307_RTC writes a single byte on given address
// Address can have any value fromm 0 to 0xFF, and DataByte can have a value of 0 to 0xFF.
void Write_Byte_To_DS1307_RTC(unsigned char Address, unsigned char DataByte)
{
	I2C_Start();										// Start i2c communication

	// Send i2c address of DS1307 with write command
	while(I2C_Write_Byte(Device_Address_DS1307_EEPROM + 0) == 1)// Wait until device is free
	{	I2C_Start();	}		

	I2C_Write_Byte(Address);							// Write Address byte
	I2C_Write_Byte(DataByte);							// Write data byte
	I2C_Stop();											// Stop i2c communication
}



// Function Purpose: Read_Byte_From_DS1307_RTC reads a single byte from given address
// Address can have any value fromm 0 to 0xFF.
unsigned char Read_Byte_From_DS1307_RTC(unsigned char Address)
{
	unsigned char Byte = 0;								// Variable to store Received byte

	I2C_Start();										// Start i2c communication

	// Send i2c address of DS1307 with write command
	while(I2C_Write_Byte(Device_Address_DS1307_EEPROM + 0) == 1)// Wait until device is free
	{	I2C_Start();	}		

	I2C_Write_Byte(Address);							// Write Address byte
	I2C_ReStart();										// Restart i2c

	// Send i2c address of DS1307 RTC with read command	
	I2C_Write_Byte(Device_Address_DS1307_EEPROM + 1);		

	Byte = I2C_Read_Byte();								// Read byte from EEPROM

	// Make SCK low, so that slave can stop driving SDA pin
	// Send a NACK to indiacate single byte read is complete
	I2C_Send_NACK();

	// Send start bit and then stop bit to stop transmission
	Set_SDA_Low;				// Make SDA Low
	__delay_us(HalfBitDelay);	// Half bit delay
	Set_SDA_High;				// Make SDA high
	__delay_us(HalfBitDelay);	// Half bit delay

	return Byte;				// Return the byte received from 24LC64 EEPROM
}



// Function Purpose: Write_Bytes_To_DS1307_RTC writes mulitple bytes from given starting address.
// Address can have any value fromm 0 to 0xFF and pData is pointer to the array
// containing NoOfBytes bytes in it. NoOfBytes is the number of bytes to write.
void Write_Bytes_To_DS1307_RTC(unsigned char Address,unsigned char* pData,unsigned char NoOfBytes)
{
	unsigned int i;

	I2C_Start();										// Start i2c communication

	// Send i2c address of DS1307 with write command
	while(I2C_Write_Byte(Device_Address_DS1307_EEPROM + 0) == 1)// Wait until device is free
	{	I2C_Start();	}		

	I2C_Write_Byte(Address);							// Write Address byte

	for(i=0;i<NoOfBytes;i++)							// Write NoOfBytes
		I2C_Write_Byte(pData[i]);						// Write data byte

	I2C_Stop();											// Stop i2c communication
}




// Function Purpose: Read_Bytes_From_DS1307_RTC reads a NoOfBytes bytes from given starting address.
// Address can have any value fromm 0 to 0xFF. NoOfBytes is the number of bytes to write.
// Read bytes are returned in pData array.
void Read_Bytes_From_DS1307_RTC(unsigned char Address, unsigned char* pData, unsigned int NoOfBytes)
{
	unsigned int i;

	I2C_Start();										// Start i2c communication

	// Send i2c address of DS1307 with write command
	while(I2C_Write_Byte(Device_Address_DS1307_EEPROM + 0) == 1)// Wait until device is free
	{	I2C_Start();	}		

	I2C_Write_Byte(Address);							// Write Address byte
	I2C_ReStart();										// Restart i2c

	// Send i2c address of DS1307 RTC with read command	
	I2C_Write_Byte(Device_Address_DS1307_EEPROM + 1);			

	pData[0] = I2C_Read_Byte();							// Read First byte from EEPROM

	for(i=1;i<NoOfBytes;i++)							// Read NoOfBytes
	{		
		I2C_Send_ACK();					// Give Ack to slave to start receiving next byte
		pData[i] = I2C_Read_Byte();		// Read next byte from EEPROM
	}

	// Make SCK low, so that slave can stop driving SDA pin
	// Send a NACK to indiacate read operation is complete
	I2C_Send_NACK();

	// Send start bit and then stop bit to stop transmission
	Set_SDA_Low;				// Make SDA Low
	__delay_us(HalfBitDelay);	// Half bit delay
	Set_SDA_High;				// Make SDA high
	__delay_us(HalfBitDelay);	// Half bit delay
}




// Function Purpose: Set_DS1307_RTC_Time sets given time in RTC registers.
// Mode can have a value AM_Time or PM_Time	or TwentyFourHoursMode only.
// Hours can have value from 0 to 23 only.
// Mins can have value from 0 to 59 only.
// Secs can have value from 0 to 59 only.
void Set_DS1307_RTC_Time(unsigned char Mode, unsigned char Hours, unsigned char Mins, unsigned char Secs)
{
	// Convert Hours, Mins, Secs into BCD
	pRTCArray[0] = (((unsigned char)(Secs/10))<<4)|((unsigned char)(Secs%10));
	pRTCArray[1] = (((unsigned char)(Mins/10))<<4)|((unsigned char)(Mins%10));
	pRTCArray[2] = (((unsigned char)(Hours/10))<<4)|((unsigned char)(Hours%10));

	switch(Mode)	// Set mode bits
	{
	case AM_Time: 	pRTCArray[2] |= 0x40;	break;
	case PM_Time: 	pRTCArray[2] |= 0x60;	break;
	
	default:	break;	// do nothing for 24HoursMode
	}

	// WritepRTCArray to DS1307
	Write_Bytes_To_DS1307_RTC(0x00, pRTCArray, 3);
}





// Function Purpose: Get_DS1307_RTC_Time returns current time from RTC registers.
// Pointer to pRTCArray is returned, in this array
// pRTCArray[3] can have a value AM_Time or PM_Time	or TwentyFourHoursMode only.
// pRTCArray[2] (Hours byte) can have value from 0 to 23 only.
// pRTCArray[1] (Mins byte) can have value from 0 to 59 only.
// pRTCArray[0] (Secs byte) can have value from 0 to 59 only.
unsigned char* Get_DS1307_RTC_Time(void)
{
	// Read Hours, Mins, Secs register from RTC
	Read_Bytes_From_DS1307_RTC(0x00, pRTCArray, 3);

	// Convert Secs back from BCD into number
	Temp = pRTCArray[0];
	pRTCArray[0] = ((Temp&0x7F)>>4)*10 + (Temp&0x0F);

	// Convert Mins back from BCD into number
	Temp = pRTCArray[1];
	pRTCArray[1] = (Temp>>4)*10 + (Temp&0x0F);

	// Convert Hours back from BCD into number
	if(pRTCArray[2]&0x40)	// if 12 hours mode
	{
		if(pRTCArray[2]&0x20)	// if PM Time
			pRTCArray[3] = PM_Time;
		else		// if AM time
			pRTCArray[3] = AM_Time;

		Temp = pRTCArray[2];
		pRTCArray[2] = ((Temp&0x1F)>>4)*10 + (Temp&0x0F);
	}
	else		// if 24 hours mode
	{
		Temp = pRTCArray[2];
		pRTCArray[2] = (Temp>>4)*10 + (Temp&0x0F);
		pRTCArray[3] = TwentyFourHoursMode;
	}

	return pRTCArray;
}





// Function Purpose: Set_DS1307_RTC_Date sets given date in RTC registers.
// Year can have a value from 0 to 99 only.
// Month can have value from 1 to 12 only.
// Date can have value from 1 to 31 only.
// Day can have value from 1 to 7 only. Where 1 means Monday, 2 means Tuesday etc.
void Set_DS1307_RTC_Date(unsigned char Date, unsigned char Month, unsigned char Year, unsigned char Day)
{
	// Convert Year, Month, Date, Day into BCD
	pRTCArray[0] = (((unsigned char)(Day/10))<<4)|((unsigned char)(Day%10));
	pRTCArray[1] = (((unsigned char)(Date/10))<<4)|((unsigned char)(Date%10));
	pRTCArray[2] = (((unsigned char)(Month/10))<<4)|((unsigned char)(Month%10));
	pRTCArray[3] = (((unsigned char)(Year/10))<<4)|((unsigned char)(Year%10));

	// WritepRTCArray to DS1307
	Write_Bytes_To_DS1307_RTC(0x03, pRTCArray, 4);
}




// Function Purpose: Get_DS1307_RTC_Date returns current date from RTC registers.
// Pointer to pRTCArray is returned, in this array
// pRTCArray[3] (Year byte) can have value from 0 to 99 only.
// pRTCArray[2] (Month byte) can have value from 1 to 12 only.
// pRTCArray[1] (Date byte) can have value from 1 to 31 only.
// pRTCArray[0] (Day byte) can have value from 1 to 7 only.
unsigned char* Get_DS1307_RTC_Date(void)
{
	// Read Hours, Mins, Secs register from RTC
	Read_Bytes_From_DS1307_RTC(0x03, pRTCArray, 4);

	// Convert Date back from BCD into number
	Temp = pRTCArray[1];
	pRTCArray[1] = (Temp>>4)*10 + (Temp&0x0F);

	// Convert Month back from BCD into number
	Temp = pRTCArray[2];
	pRTCArray[2] = (Temp>>4)*10 + (Temp&0x0F);

	// Convert Year back from BCD into number
	Temp = pRTCArray[3];
	pRTCArray[3] = (Temp>>4)*10 + (Temp&0x0F);

	return pRTCArray;
}