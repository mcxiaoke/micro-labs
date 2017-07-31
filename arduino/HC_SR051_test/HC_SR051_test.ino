
const byte SR_PIN = A0;
const byte LED_PIN = 9;
int value = 0;

void setup() {
  pinMode(SR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  value = digitalRead(SR_PIN);
  if (value == 1) {
    Serial.println("Found!");
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  delay(500);
}
