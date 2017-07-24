/*
 * Author: Luka Ilic
 * E-mail: Luka_Bn@hotmail.com
 * youtube.com/Luka9840
 * 3x 74HC595 (+) 
 *
 */

const int ds = 2;
const int stcp = 3;
const int shcp = 5;

void setup() {
  pinMode(ds, OUTPUT);
  pinMode(stcp, OUTPUT);
  pinMode(shcp, OUTPUT);
  ukljucenje();
}

boolean digitalniPin[24]; //Ukupan broj digitalnih pinova

void ukljucenje() {
  digitalWrite(stcp, LOW);
  for(int i = 23; i>=0; i--) { //Broj pinova -1, jer brojanje pocinje od 0
    digitalWrite(shcp, LOW);
    digitalWrite(ds, digitalniPin[i] );
    digitalWrite(shcp, HIGH);
  }
  digitalWrite(stcp, HIGH);
}

void loop() {
  for(int i = 0; i<=23; i++) {
    digitalniPin[i] = HIGH; 
    delay(100); 
    ukljucenje(); 
  } 
  for(int i = 23; i>=0; i--) {
    digitalniPin[i] = LOW;
    delay(100);
    ukljucenje();
  }
}
