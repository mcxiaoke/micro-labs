/*
Using the 74HC595 to control 8 leds (outputs) without giving up 8 ports on the arduino uno. 
Three outputs from the arduino are used to shift the data out in serial to the 74HC595 were it 
is then parceled out to each of its individual pins. 

*/

int latchPin = 5;   // to pin 12
int clockPin = 6;	// to pin 11
int dataPin = 4;	// to pin 14

byte leds = 0;
int pattern[] = {1, 2, 4, 8, 16, 32, 64, 128, 64, 32, 16, 8, 4, 2};

void setup() 
{
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
}

void loop() 
{
	
  	for (int i = 0; i < 14; i++)
  	{
    	leds = pattern[i];
    	updateShiftRegister();
    	delay(1000);
  	}
}

void updateShiftRegister()
{
   	digitalWrite(latchPin, LOW);
   	shiftOut(dataPin, clockPin, LSBFIRST, leds);
   	digitalWrite(latchPin, HIGH);
}