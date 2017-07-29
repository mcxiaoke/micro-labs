#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0xBF);  // Set the LCD I2C address

#define BACKLIGHT_PIN     13

void setup()
{
  // Switch on the backlight
  pinMode ( BACKLIGHT_PIN, OUTPUT );
  digitalWrite ( BACKLIGHT_PIN, HIGH );
  
  lcd.begin(16,2);               // initialize the lcd 
 lcd.backlight();
  lcd.home ();                   // go home
  lcd.print("Hello, ARDUINO ");  
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print (" WORLD!  ");      
}

void loop()
{

}
