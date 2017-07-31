const byte ledPin = 9;
const byte pot = A0;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(pot, INPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.println(analogRead(pot));
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    analogWrite(ledPin, fadeValue);
    delay(analogRead(pot) / 5);
  }
  Serial.println(analogRead(pot));
  for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(ledPin, fadeValue);
    delay(analogRead(pot) / 5);
  }
}
