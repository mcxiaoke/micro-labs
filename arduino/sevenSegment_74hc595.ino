/*
Shift Register Wiring Schematic
 Q1 -|    |- VCC
 Q2 -|    |- Q0
 Q3 -|    |- DS
 Q4 -|    |- OE
 Q5 -|    |- ST_CP
 Q6 -|    |- SH_CP
 Q7 -|    |- MR
VSS -|    |- Q' 
 
Key:
Q0 - Q7: Parallel Out Pins
Q': Cascade Pin
DS: Serial Data In Pin
OE: Output Enable (Active Low)
ST_CP: Latch Pin
SH_CP: Clock Pin
MR: Master Reset  (Active Low)
*/
 
// ST_CP of 74HC595
const int LATCH_PIN = 8;
// SH_CP of 74HC595
const int CLOCK_PIN = 12;
// DS of 74HC595
const int DATA_PIN = 11;
 
// Hex Character from 0 - F

byte hexArray[4] = 
{ 
          B00001100, // #1
          B01110110, // #2
          B01011110, // #3
          B11001100, // #4
          
//        B01000000, 
//        B00100000, 
//        B00010000, 
//        B00001000,
//  	  B00000100, 
//        B00000010,
//        B10000000, 
//        B01000000,

};
 
void setup() 
{
  Serial.begin(9600);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
}
 

 
void loop() 
{
  for(int i=0; i <= 4; ++i){
    sendToShiftRegister(i);
    delay(1000);
  }
}

void sendToShiftRegister(int pot)
{
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, hexArray[pot]);   
    digitalWrite(LATCH_PIN, HIGH);
    delay(20);
}
 
int getAnalog()
{
  int getAnalog = analogRead(0)/64;
  //Serial.println(getAnalog);
  return getAnalog;
}
