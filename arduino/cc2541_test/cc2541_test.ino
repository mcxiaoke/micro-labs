#include <SoftwareSerial.h>

#define PTX       8
#define PRX       9
SoftwareSerial ble(PRX, PTX);   //RX, TX

String _comdata_ = "";//for incoming serial data

void setup() {
  Serial.begin(9600);
  ble.begin(9600);
  Serial.println("System is Ready!");
  delay(200);
  ble.println("AT+NAME");
}

void loop() {
  getSerialData();
  if (Serial.available()) {
    String order = "";
    while (Serial.available()) {
      char cc = (char)Serial.read();
      order += cc;
      delay(2);
    }
    order.trim();
    ble.println(order);
  }
  if (_comdata_ != "") {
    Serial.println(_comdata_);
    _comdata_ = String("");
  }
}

void getSerialData() {
  while (ble.available() > 0) {
    _comdata_ += char(ble.read());   //get serial data
    delay(5);
  }
}
