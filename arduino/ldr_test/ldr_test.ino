// 光敏电阻
// 需要串联一个电阻
// 最暗的时候1024
// 最亮的时候 20
// 普通灯光 840
void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.println(analogRead(A0));
  delay(1000);
}
