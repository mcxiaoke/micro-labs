const byte IN1 = 7;
const byte IN2 = 8;
const byte ENA = 9;

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(13, OUTPUT);
  Serial.begin(9600);
}

void startOnce() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(2000);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  delay(2000);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  delay(2000);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  delay(2000);
}

void ledBlink() {
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(500);
}

int i = 0;
void loop() {
  i += 20;
  int speed = i % 255;
  Serial.println(speed);
  analogWrite(ENA, 125);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(1000);
}
