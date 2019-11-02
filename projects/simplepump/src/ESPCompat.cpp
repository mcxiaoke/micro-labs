#include "ESPCompat.h"

String listFiles() {
  String output = "";
  Serial.println(F("[System] SPIFFS Files:"));
#if defined(ESP8266)

  Dir f = SPIFFS.openDir("/");
  while (f.next()) {
    Serial.printf("[File] %s (%d bytes)\n", f.fileName().c_str(), f.fileSize());
    output += f.fileName();
    output += "\n";
  };

#elif defined(ESP32)
  File root = SPIFFS.open("/");
  if (root.isDirectory()) {
    File f = root.openNextFile();
    while (f) {
      Serial.printf("[File] %s (%d bytes)\n", f.name(), f.size());
      output += f.name();
      // output += " (";
      // output += f.fileSize();
      // output += ")";
      output += "\n";
      f = root.openNextFile();
    };
  }

#endif
  return output;
}

void fsCheck() {
#if defined(ESP8266)
  if (!SPIFFS.begin()) {
#elif defined(ESP32)
  if (!SPIFFS.begin(true)) {
#endif
    Serial.println(F("[System] Failed to mount file system"));
  } else {
    Serial.println(F("[System] SPIFFS file system mounted."));
  }
}