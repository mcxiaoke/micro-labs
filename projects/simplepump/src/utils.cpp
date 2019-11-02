#include "utils.h"

String getDevice() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return mac.substring(mac.length() / 2);
}

String getMD5(uint8_t* data, uint16_t len) {
  MD5Builder md5;
  md5.begin();
  md5.add(data, len);
  md5.calculate();
  return md5.toString();
}

String getMD5(const char* data) {
  MD5Builder md5;
  md5.begin();
  md5.add(data);
  md5.calculate();
  return md5.toString();
}

String getMD5(const String& data) {
  return getMD5(data.c_str());
}

void showESP() {
#if defined(ESP8266)
  Serial.printf("Free Stack: %d, Free Heap: %d\n", ESP.getFreeContStack(),
                ESP.getFreeHeap());
#elif defined(ESP32)
  Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
#endif
}